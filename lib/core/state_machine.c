/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/state_machine.h>
#include <string.h>

#define ARM_DEBOUNCE_MS 1000

void sm_init(sm_state_t *state)
{
	memset(state, 0, sizeof(*state));
	state->mode = DRONE_MODE_CALIBRATE;
	state->calibration_valid = false;
	state->last_disarm_ms = 0;
}

void sm_apply(sm_state_t *state, const sm_result_t *result)
{
	state->mode = result->next_mode;
	state->calibration_valid = result->calibration_valid;
	state->last_disarm_ms = result->last_disarm_ms;
}

static sm_result_t make_result(const sm_state_t *state, drone_mode_t mode)
{
	sm_result_t r;

	memset(&r, 0, sizeof(r));
	r.next_mode = mode;
	r.calibration_valid = state->calibration_valid;
	r.last_disarm_ms = state->last_disarm_ms;
	return r;
}

static void add_action(sm_result_t *r, drone_action_t action)
{
	if (r->action_count < MAX_ACTIONS) {
		r->actions[r->action_count++] = action;
	}
}

static sm_result_t process_calibrate(const sm_state_t *state,
				     drone_event_t event)
{
	sm_result_t r = make_result(state, DRONE_MODE_CALIBRATE);

	switch (event) {
	case EVENT_CALIB_OK:
		r.next_mode = DRONE_MODE_DIAG;
		r.calibration_valid = true;
		add_action(&r, ACTION_REPORT_CALIB_OK);
		add_action(&r, ACTION_REPORT_MODE);
		break;
	case EVENT_CALIB_FAIL:
		add_action(&r, ACTION_REPORT_CALIB_FAIL);
		break;
	case EVENT_CALIB_TIMEOUT:
		add_action(&r, ACTION_REPORT_CALIB_FAIL);
		break;
	case EVENT_LOW_BATT_CUTOFF:
		r.next_mode = DRONE_MODE_DIAG;
		add_action(&r, ACTION_MOTORS_OFF);
		add_action(&r, ACTION_REPORT_LOWBATT);
		break;
	default:
		break;
	}
	return r;
}

static sm_result_t process_diag(const sm_state_t *state, drone_event_t event,
				uint32_t now_ms)
{
	sm_result_t r = make_result(state, DRONE_MODE_DIAG);

	switch (event) {
	case EVENT_ARM:
		if (state->calibration_valid &&
		    (now_ms - state->last_disarm_ms) >= ARM_DEBOUNCE_MS) {
			r.next_mode = DRONE_MODE_RUNNING;
			add_action(&r, ACTION_REPORT_MODE);
		} else {
			add_action(&r, ACTION_REPORT_ERROR);
		}
		break;
	case EVENT_RECALIBRATE:
		r.next_mode = DRONE_MODE_CALIBRATE;
		r.calibration_valid = false;
		add_action(&r, ACTION_REPORT_MODE);
		break;
	case EVENT_LOW_BATT_CUTOFF:
		add_action(&r, ACTION_MOTORS_OFF);
		add_action(&r, ACTION_REPORT_LOWBATT);
		break;
	default:
		break;
	}
	return r;
}

static sm_result_t process_running(const sm_state_t *state,
				   drone_event_t event, uint32_t now_ms)
{
	sm_result_t r = make_result(state, DRONE_MODE_RUNNING);

	switch (event) {
	case EVENT_DISARM:
	case EVENT_SW2_PRESS:
		r.next_mode = DRONE_MODE_DIAG;
		r.last_disarm_ms = now_ms;
		add_action(&r, ACTION_MOTORS_RAMP_DOWN);
		add_action(&r, ACTION_PID_RESET);
		add_action(&r, ACTION_REPORT_MODE);
		break;
	case EVENT_BT_DISCONNECT:
	case EVENT_IMU_FAILURE:
		r.next_mode = DRONE_MODE_DIAG;
		r.last_disarm_ms = now_ms;
		add_action(&r, ACTION_MOTORS_RAMP_DOWN);
		add_action(&r, ACTION_PID_RESET);
		add_action(&r, ACTION_REPORT_MODE);
		break;
	case EVENT_LOW_BATT_CRITICAL:
		r.next_mode = DRONE_MODE_DIAG;
		r.last_disarm_ms = now_ms;
		add_action(&r, ACTION_MOTORS_RAMP_DOWN);
		add_action(&r, ACTION_PID_RESET);
		add_action(&r, ACTION_REPORT_LOWBATT);
		break;
	case EVENT_LOW_BATT_CUTOFF:
	case EVENT_EXCESSIVE_TILT:
		r.next_mode = DRONE_MODE_DIAG;
		r.last_disarm_ms = now_ms;
		add_action(&r, ACTION_MOTORS_OFF);
		add_action(&r, ACTION_PID_RESET);
		add_action(&r, ACTION_REPORT_MODE);
		break;
	default:
		break;
	}
	return r;
}

sm_result_t sm_process(const sm_state_t *state, drone_event_t event,
		       uint32_t now_ms)
{
	switch (state->mode) {
	case DRONE_MODE_CALIBRATE:
		return process_calibrate(state, event);
	case DRONE_MODE_DIAG:
		return process_diag(state, event, now_ms);
	case DRONE_MODE_RUNNING:
		return process_running(state, event, now_ms);
	default: {
		sm_result_t r = make_result(state, DRONE_MODE_DIAG);
		add_action(&r, ACTION_MOTORS_OFF);
		add_action(&r, ACTION_REPORT_ERROR);
		return r;
	}
	}
}
