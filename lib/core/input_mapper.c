/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/input_mapper.h>

int16_t input_map_joystick(uint16_t raw)
{
	if (raw > JOYSTICK_MAX) {
		raw = JOYSTICK_MAX;
	}
	return (int16_t)raw - JOYSTICK_CENTER;
}

static uint8_t clamp_slider(uint8_t value)
{
	return (value > 100) ? 100 : value;
}

float input_map_kp(uint8_t value)
{
	return clamp_slider(value) * 0.05f;
}

float input_map_ki(uint8_t value)
{
	return clamp_slider(value) * 0.02f;
}

float input_map_kd(uint8_t value)
{
	return clamp_slider(value) * 0.01f;
}
