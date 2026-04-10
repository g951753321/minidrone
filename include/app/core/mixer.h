/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_MIXER_H
#define APP_CORE_MIXER_H

#include <stdint.h>

#define MAX_THROTTLE  3599
#define MOTOR_IDLE_DUTY 50

/* Motor indices */
#define MOTOR_FR 0
#define MOTOR_FL 1
#define MOTOR_BR 2
#define MOTOR_BL 3

/**
 * Compute motor duty values from throttle and PID outputs.
 * Pure function: all outputs clamped to [0, MAX_THROTTLE].
 *
 * @param throttle  Base throttle (0 to MAX_THROTTLE)
 * @param pitch     PID pitch output
 * @param roll      PID roll output
 * @param yaw       PID yaw output
 * @param duty_out  Output array of 4 duty values [FR, FL, BR, BL]
 */
void mixer_update(float throttle, float pitch, float roll, float yaw,
		  uint16_t duty_out[4]);

#endif
