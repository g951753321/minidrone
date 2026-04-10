/* SPDX-License-Identifier: Apache-2.0 */

#include <app/shell.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(stat0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(stat1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(stat2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(stat3), gpios),
};

#define NUM_LEDS ARRAY_SIZE(leds)

int shell_led_init(void)
{
	for (int i = 0; i < NUM_LEDS; i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			return -ENODEV;
		}
		int ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

void shell_led_set(uint8_t led_idx, bool on)
{
	if (led_idx < NUM_LEDS) {
		gpio_pin_set_dt(&leds[led_idx], on ? 1 : 0);
	}
}

void shell_led_toggle(uint8_t led_idx)
{
	if (led_idx < NUM_LEDS) {
		gpio_pin_toggle_dt(&leds[led_idx]);
	}
}
