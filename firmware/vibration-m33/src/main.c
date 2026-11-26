/*
 * M33 firmware of the vibration node.
 * Wire test: read the ADXL345's ID register over SPI1 and show the result on LD3.
 */

#include <zephyr/kernel.h>
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

#define ADXL345_REG_DEVID   0x00
#define ADXL345_DEVID       0xE5   /* fixed value (ADXL345 Rev. G, p. 24) */
#define ADXL345_READ        0x80   /* bit 7 of the first byte = read (p. 14) */

/* Read one register: send [read | address, dummy], the second received byte is the value. */
static int adxl345_read_reg(uint8_t reg, uint8_t *value)
{
	uint8_t tx[2] = { ADXL345_READ | reg, 0x00 };
	uint8_t rx[2] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx, .len = sizeof(tx) };
	const struct spi_buf rx_buf = { .buf = rx, .len = sizeof(rx) };
	const struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

	int ret = spi_transceive_dt(&accel, &tx_set, &rx_set);

	*value = rx[1];
	return ret;
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
	 *   LED on, steady   : SPI1 did not start (its init failed)
	 *   1 flash          : the transfer itself failed
	 *   2 flashes        : transfer done, but the ID is wrong (wiring, mode)
	 *   3 flashes        : the ID is 0xE5: the M33 talks to the ADXL345
	 */
	if (!spi_is_ready_dt(&accel)) {
		gpio_pin_set_dt(&led, 1);
		return 0;
	}

	if (adxl345_read_reg(ADXL345_REG_DEVID, &devid) != 0) {
		result = 1;
	} else if (devid != ADXL345_DEVID) {
		result = 2;
	} else {
		result = 3;
	}

	while (1) {
		flashes(result);
	}
	return 0;
}
