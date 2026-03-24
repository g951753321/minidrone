/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_COMMAND_H
#define APP_CORE_COMMAND_H

#include <stdint.h>

/**
 * Abstract input command interface.
 *
 * All input sources (MicroBlue, RC receiver, USB, etc.) translate their
 * transport-specific messages into drone_cmd_t before passing to the
 * flight controller. This decouples the core logic from any specific
 * input backend.
 */

typedef enum {
	CMD_NONE,
	CMD_ARM,
	CMD_DISARM,
	CMD_CALIBRATE,
	CMD_JOYSTICK,
	CMD_SET_PID_GAIN,
	CMD_MOTOR_TEST,
} cmd_type_t;

typedef enum {
	GAIN_KP,
	GAIN_KI,
	GAIN_KD,
} gain_type_t;

typedef struct {
	cmd_type_t type;
	union {
		struct {
			uint8_t axis;  /* 0 = throttle/yaw, 1 = pitch/roll */
			int16_t x;
			int16_t y;
		} joystick;
		struct {
			gain_type_t gain;
			uint8_t value; /* 0-100 */
		} pid_gain;
		struct {
			uint8_t motor_id; /* 0-3: FR, FL, BR, BL */
		} motor_test;
	};
} drone_cmd_t;

/**
 * Telemetry types for output interface.
 */
typedef enum {
	TELEM_BATTERY,
	TELEM_CALIB_RESULT,
	TELEM_IMU,
	TELEM_MODE,
	TELEM_ERROR,
	TELEM_LOW_BATT,
} telem_type_t;

#endif
