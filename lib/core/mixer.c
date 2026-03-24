/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/mixer.h>

static uint16_t clamp(float val)
{
	if (val < 0.0f) {
		return 0;
	}
	if (val > (float)MAX_THROTTLE) {
		return MAX_THROTTLE;
	}
	return (uint16_t)val;
}

void mixer_update(float throttle, float pitch, float roll, float yaw,
		  uint16_t duty_out[4])
{
	/* FR (CW):  +pitch +roll -yaw */
	duty_out[MOTOR_FR] = clamp(throttle + pitch + roll - yaw);
	/* FL (CCW): +pitch -roll +yaw */
	duty_out[MOTOR_FL] = clamp(throttle + pitch - roll + yaw);
	/* BR (CCW): -pitch +roll +yaw */
	duty_out[MOTOR_BR] = clamp(throttle - pitch + roll + yaw);
	/* BL (CW):  -pitch -roll -yaw */
	duty_out[MOTOR_BL] = clamp(throttle - pitch - roll - yaw);
}
