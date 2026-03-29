/* SPDX-License-Identifier: Apache-2.0 */

#include <app/shell.h>
#include <app/core/mixer.h>
#include <zephyr/drivers/pwm.h>

/* TIM3 PWM controller */
static const struct device *pwm_dev;

/* Channel mapping: motor index → TIM3 channel (1-based) */
#define MOTOR_FR_CH 1 /* TIM3_CH1 = PA6 */
#define MOTOR_BL_CH 2 /* TIM3_CH2 = PA7 */
#define MOTOR_BR_CH 3 /* TIM3_CH3 = PB0 */
#define MOTOR_FL_CH 4 /* TIM3_CH4 = PB1 */

static const uint32_t motor_ch[] = {
	[MOTOR_FR] = MOTOR_FR_CH,
	[MOTOR_FL] = MOTOR_FL_CH,
	[MOTOR_BR] = MOTOR_BR_CH,
	[MOTOR_BL] = MOTOR_BL_CH,
};

#define PWM_PERIOD_NS PWM_USEC(50) /* 20 kHz = 50 us period */

int shell_motor_init(void)
{
	pwm_dev = DEVICE_DT_GET(DT_NODELABEL(motor_pwm));
	if (!device_is_ready(pwm_dev)) {
		return -ENODEV;
	}

	for (int i = 0; i < 4; i++) {
		pwm_set(pwm_dev, motor_ch[i], PWM_PERIOD_NS, 0, 0);
	}
	return 0;
}

void shell_motor_set(uint16_t duty[4])
{
	for (int i = 0; i < 4; i++) {
		uint32_t pulse_ns = (uint32_t)duty[i] * PWM_PERIOD_NS /
				    MAX_THROTTLE;
		pwm_set(pwm_dev, motor_ch[i], PWM_PERIOD_NS, pulse_ns, 0);
	}
}

void shell_motor_all_off(void)
{
	for (int i = 0; i < 4; i++) {
		pwm_set(pwm_dev, motor_ch[i], PWM_PERIOD_NS, 0, 0);
	}
}
