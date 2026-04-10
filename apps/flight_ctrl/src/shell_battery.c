/* SPDX-License-Identifier: Apache-2.0 */

#include <app/shell.h>
#include <zephyr/drivers/adc.h>

/* ADC1 channel 5 on PA5 */
static const struct device *adc_dev;

#define ADC_CHANNEL 5
#define ADC_RESOLUTION 12

static int16_t adc_buf;

static const struct adc_channel_cfg ch_cfg = {
	.gain = ADC_GAIN_1,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = ADC_CHANNEL,
};

static struct adc_sequence adc_seq = {
	.channels = BIT(ADC_CHANNEL),
	.buffer = &adc_buf,
	.buffer_size = sizeof(adc_buf),
	.resolution = ADC_RESOLUTION,
};

int shell_battery_init(void)
{
	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc1));
	if (!device_is_ready(adc_dev)) {
		return -ENODEV;
	}

	return adc_channel_setup(adc_dev, &ch_cfg);
}

uint16_t shell_battery_read_raw(void)
{
	int ret = adc_read(adc_dev, &adc_seq);

	if (ret < 0) {
		return 0;
	}
	return (uint16_t)adc_buf;
}
