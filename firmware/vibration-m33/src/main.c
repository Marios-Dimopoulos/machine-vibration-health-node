/*
 * M33 firmware of the vibration node.
 * Wire test: check that the ADXL345's ID reads over SPI1, then that its INT1 and INT2 pins
 * interrupt the M33. Show the result on LD3.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

/* LD3, through the board's led0 alias: no pin number in the code. */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/*
 * The ADXL345 node from my overlay: SPI1, its chip select, its clock limit.
 * SPI mode 3 (CPOL = 1, CPHA = 1), 8-bit words, MSB first (ADXL345 Rev. G, p. 14).
 */
static const struct spi_dt_spec accel = SPI_DT_SPEC_GET(DT_NODELABEL(accel),
		SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		SPI_MODE_CPOL | SPI_MODE_CPHA);

/* Its interrupt pins, from the same node: INT1 = PD13, INT2 = PC7. */
static const struct gpio_dt_spec int1 = GPIO_DT_SPEC_GET(DT_NODELABEL(accel), int1_gpios);
static const struct gpio_dt_spec int2 = GPIO_DT_SPEC_GET(DT_NODELABEL(accel), int2_gpios);

/* ADXL345 registers and values (ADXL345 Rev. G) */
#define ADXL345_REG_DEVID       0x00
#define ADXL345_REG_BW_RATE     0x2C   /* output data rate (p. 25) */
#define ADXL345_REG_POWER_CTL   0x2D   /* bit 3 = Measure (p. 26) */
#define ADXL345_REG_INT_ENABLE  0x2E   /* bit 7 = DATA_READY (p. 26) */
#define ADXL345_REG_INT_MAP     0x2F   /* bit = 0: to INT1, 1: to INT2 (p. 27) */
#define ADXL345_REG_DATAX0      0x32   /* 0x32 to 0x37: X, Y, Z, 2 bytes each (p. 27) */

#define ADXL345_DEVID           0xE5   /* fixed value (p. 24) */
#define ADXL345_RATE_100HZ      0x0A   /* rate code 1010 (Table 7, p. 13) */
#define ADXL345_MEASURE         0x08
#define ADXL345_DATA_READY      0x80

#define ADXL345_READ            0x80   /* first byte, bit 7 = 1: read (p. 14) */
#define ADXL345_MULTI           0x40   /* first byte, bit 6 = 1: several registers in a row */

#define TEST_TIME_MS            2000   /* how long each pin is watched */

/* Interrupt counters, one per pin. Changed in the interrupt handlers, so atomic. */
static atomic_t int1_count;
static atomic_t int2_count;

/* Given by the interrupt handlers, taken by main(): "new data is ready" */
static K_SEM_DEFINE(data_ready, 0, 1);

static struct gpio_callback int1_cb;
static struct gpio_callback int2_cb;

/* Wrap one array as the buffer set the SPI API wants. */
#define ONE_BUF(name, array) \
	const struct spi_buf name##_buf = { .buf = (array), .len = sizeof(array) }; \
	const struct spi_buf_set name = { .buffers = &name##_buf, .count = 1 }

/* Read one register: send [read | address, dummy], the second received byte is the value. */
static int adxl345_read_reg(uint8_t reg, uint8_t *value)
{
	uint8_t tx[2] = { ADXL345_READ | reg, 0x00 };
	uint8_t rx[2] = { 0 };
	ONE_BUF(tx_set, tx);
	ONE_BUF(rx_set, rx);

	int ret = spi_transceive_dt(&accel, &tx_set, &rx_set);

	*value = rx[1];
	return ret;
}

/* Write one register: send [address (bit 7 = 0: write), value]. Nothing to receive. */
static int adxl345_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t tx[2] = { reg, value };
	ONE_BUF(tx_set, tx);

	return spi_write_dt(&accel, &tx_set);
}

/*
 * Read the 6 data registers in one transfer. The values are thrown away: reading them is what
 * clears DATA_READY, so the pin goes low and the next measurement gives a new rising edge (p. 19).
 */
static int adxl345_read_data(void)
{
	uint8_t tx[7] = { ADXL345_READ | ADXL345_MULTI | ADXL345_REG_DATAX0 };
	uint8_t rx[7];
	ONE_BUF(tx_set, tx);
	ONE_BUF(rx_set, rx);

	return spi_transceive_dt(&accel, &tx_set, &rx_set);
}

/*
 * The interrupt handlers. They run in interrupt context, so they must be short and must never
 * wait: they only count and signal. The SPI read happens in main(), a thread.
 */
static void int1_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	atomic_inc(&int1_count);
	k_sem_give(&data_ready);
}

static void int2_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	atomic_inc(&int2_count);
	k_sem_give(&data_ready);
}

/* Set one pin as an input that interrupts on its rising edge. */
static int setup_int_pin(const struct gpio_dt_spec *pin, struct gpio_callback *cb,
			 gpio_callback_handler_t handler)
{
	if (!gpio_is_ready_dt(pin)) {
		return -ENODEV;
	}
	if (gpio_pin_configure_dt(pin, GPIO_INPUT) != 0) {
		return -EIO;
	}
	gpio_init_callback(cb, handler, BIT(pin->pin));
	if (gpio_add_callback_dt(pin, cb) != 0) {
		return -EIO;
	}
	return gpio_pin_interrupt_configure_dt(pin, GPIO_INT_EDGE_TO_ACTIVE);
}

/*
 * Send DATA_READY to one pin (int_map = 0x00: INT1, 0x80: INT2) and serve it for TEST_TIME_MS.
 * The datasheet asks to disable an interrupt before changing its mapping (p. 19).
 */
static void run_phase(uint8_t int_map)
{
	adxl345_write_reg(ADXL345_REG_INT_ENABLE, 0x00);
	adxl345_write_reg(ADXL345_REG_INT_MAP, int_map);
	adxl345_write_reg(ADXL345_REG_INT_ENABLE, ADXL345_DATA_READY);

	int64_t end = k_uptime_get() + TEST_TIME_MS;

	while (k_uptime_get() < end) {
		/*
		 * Wait for an interrupt, at most 100 ms. Read the data either way: if a measurement
		 * was already waiting before the pin was watched, its edge was missed and the pin
		 * would stay high forever. Reading clears it.
		 */
		k_sem_take(&data_ready, K_MSEC(100));
		adxl345_read_data();
	}
}

/* Blink n short flashes, then a pause. */
static void flashes(int n)
{
	for (int i = 0; i < n; i++) {
		gpio_pin_set_dt(&led, 1);
		k_msleep(100);
		gpio_pin_set_dt(&led, 0);
		k_msleep(200);
	}
	k_msleep(1000);
}

int main(void)
{
	uint8_t devid = 0;
	int result;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	/*
	 * The result, shown on LD3:
	 *   LED on, steady : setup failed (SPI1, the ID, or the interrupt pins)
	 *   1 flash        : INT1 gave no interrupts
	 *   2 flashes      : INT2 gave no interrupts
	 *   3 flashes      : neither gave any
	 *   4 flashes      : both pins interrupt the M33
	 */
	if (!spi_is_ready_dt(&accel) ||
	    adxl345_read_reg(ADXL345_REG_DEVID, &devid) != 0 || devid != ADXL345_DEVID ||
	    setup_int_pin(&int1, &int1_cb, int1_handler) != 0 ||
	    setup_int_pin(&int2, &int2_cb, int2_handler) != 0) {
		gpio_pin_set_dt(&led, 1);
		return 0;
	}

	/* Sensor setup: standby while configuring, 100 measurements per second, then measure. */
	adxl345_write_reg(ADXL345_REG_POWER_CTL, 0x00);
	adxl345_write_reg(ADXL345_REG_BW_RATE, ADXL345_RATE_100HZ);
	adxl345_write_reg(ADXL345_REG_POWER_CTL, ADXL345_MEASURE);

	/* Phase 1: DATA_READY on INT1. Phase 2: DATA_READY on INT2. */
	run_phase(0x00);
	atomic_val_t on_int1 = atomic_get(&int1_count);

	run_phase(ADXL345_DATA_READY);
	atomic_val_t on_int2 = atomic_get(&int2_count);

	adxl345_write_reg(ADXL345_REG_INT_ENABLE, 0x00);

	if (on_int1 > 0 && on_int2 > 0) {
		result = 4;
	} else if (on_int1 == 0 && on_int2 == 0) {
		result = 3;
	} else if (on_int2 == 0) {
		result = 2;
	} else {
		result = 1;
	}

	while (1) {
		flashes(result);
	}
	return 0;
}
