/*
 * M33 firmware of the vibration node.
 * First version: blink LD3 with my own pattern, to prove the app builds and runs from its own
 * directory.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* LD3, through the board's led0 alias: no pin number in the code. */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	/* Two short flashes, then a pause: easy to tell apart from Zephyr's blinky. */
	while (1) {
		for (int i = 0; i < 2; i++) {
			gpio_pin_set_dt(&led, 1);
			k_msleep(100);
			gpio_pin_set_dt(&led, 0);
			k_msleep(150);
		}
		k_msleep(1000);
	}
	return 0;
}
