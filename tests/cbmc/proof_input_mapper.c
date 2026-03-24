/* SPDX-License-Identifier: Apache-2.0
 *
 * CBMC proof harness for input mapper.
 * Verifies VP-09 (center = 0) and VP-23 (gain mapping bounds).
 *
 * Run: cbmc -I../../include ../../lib/core/input_mapper.c proof_input_mapper.c
 */

#include <app/core/input_mapper.h>
#include <assert.h>

/* VP-09: Joystick center (512) produces exactly zero */
void proof_joystick_center(void)
{
	int16_t result = input_map_joystick(JOYSTICK_CENTER);

	assert(result == 0);
}

/* Joystick output always in [-512, +511] for any uint16 input */
void proof_joystick_bounds(void)
{
	uint16_t raw;

	int16_t result = input_map_joystick(raw);

	assert(result >= -512);
	assert(result <= 511);
}

/* VP-23: Kp mapping always in [0.0, 5.0] */
void proof_kp_bounds(void)
{
	uint8_t value;

	float kp = input_map_kp(value);

	assert(kp >= 0.0f);
	assert(kp <= 5.0f);
}

/* VP-23: Ki mapping always in [0.0, 2.0] */
void proof_ki_bounds(void)
{
	uint8_t value;

	float ki = input_map_ki(value);

	assert(ki >= 0.0f);
	assert(ki <= 2.0f);
}

/* VP-23: Kd mapping always in [0.0, 1.0] */
void proof_kd_bounds(void)
{
	uint8_t value;

	float kd = input_map_kd(value);

	assert(kd >= 0.0f);
	assert(kd <= 1.0f);
}

int main(void)
{
	proof_joystick_center();
	proof_joystick_bounds();
	proof_kp_bounds();
	proof_ki_bounds();
	proof_kd_bounds();
	return 0;
}
