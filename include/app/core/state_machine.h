/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_STATE_MACHINE_H
#define APP_CORE_STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	DRONE_MODE_CALIBRATE,
	DRONE_MODE_DIAG,
	DRONE_MODE_RUNNING,
} drone_mode_t;

typedef enum {
	EVENT_CALIB_OK,
	EVENT_CALIB_FAIL,
	EVENT_CALIB_TIMEOUT,
	EVENT_ARM,
	EVENT_DISARM,
	EVENT_RECALIBRATE,
	EVENT_BT_DISCONNECT,
	EVENT_LOW_BATT_CRITICAL,
	EVENT_LOW_BATT_CUTOFF,
	EVENT_IMU_FAILURE,
	EVENT_EXCESSIVE_TILT,
	EVENT_SW2_PRESS,
} drone_event_t;

typedef enum {
	ACTION_NONE,
	ACTION_MOTORS_OFF,
	ACTION_MOTORS_RAMP_DOWN,
	ACTION_PID_RESET,
	ACTION_REPORT_CALIB_OK,
	ACTION_REPORT_CALIB_FAIL,
	ACTION_REPORT_ERROR,
	ACTION_REPORT_MODE,
	ACTION_REPORT_LOWBATT,
} drone_action_t;

#define MAX_ACTIONS 4

typedef struct {
	drone_mode_t next_mode;
	bool calibration_valid;
	uint32_t last_disarm_ms;
	drone_action_t actions[MAX_ACTIONS];
	uint8_t action_count;
} sm_result_t;

typedef struct {
	drone_mode_t mode;
	bool calibration_valid;
	uint32_t last_disarm_ms;
} sm_state_t;

/**
 * Initialize state machine to default state.
 */
void sm_init(sm_state_t *state);

/**
 * Apply sm_result_t back to sm_state_t.
 * Must be called by the effectful shell after sm_process().
 */
void sm_apply(sm_state_t *state, const sm_result_t *result);

/**
 * Process an event and return the next state + actions.
 * Pure function: no side effects.
 *
 * @param state   Current state (read-only for transition logic)
 * @param event   The event to process
 * @param now_ms  Current timestamp in ms (injected, not read from HW)
 * @return        Result with next_mode and action list
 */
sm_result_t sm_process(const sm_state_t *state, drone_event_t event,
		       uint32_t now_ms);

#endif
