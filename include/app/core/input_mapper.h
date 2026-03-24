/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_INPUT_MAPPER_H
#define APP_CORE_INPUT_MAPPER_H

#include <stdint.h>

#define JOYSTICK_CENTER 512
#define JOYSTICK_MAX    1023

/**
 * Map joystick raw value (0-1023) to signed setpoint.
 * Center (512) produces exactly 0.
 * Pure function.
 *
 * @param raw  Raw joystick axis value (0-1023)
 * @return     Signed setpoint (-512 to +511)
 */
int16_t input_map_joystick(uint16_t raw);

/**
 * Map slider value (0-100) to PID Kp gain.
 * Formula: Kp = value * 0.05, range [0.0, 5.0]
 */
float input_map_kp(uint8_t value);

/**
 * Map slider value (0-100) to PID Ki gain.
 * Formula: Ki = value * 0.02, range [0.0, 2.0]
 */
float input_map_ki(uint8_t value);

/**
 * Map slider value (0-100) to PID Kd gain.
 * Formula: Kd = value * 0.01, range [0.0, 1.0]
 */
float input_map_kd(uint8_t value);

#endif
