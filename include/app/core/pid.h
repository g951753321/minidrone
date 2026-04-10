/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_PID_H
#define APP_CORE_PID_H

#include <stdbool.h>

#define PID_WINDUP_MAX 500.0f

typedef struct {
	float kp;
	float ki;
	float kd;
	float integral;
	float prev_error;
	bool initialized;
} pid_state_t;

/**
 * Initialize PID state to zero.
 */
void pid_init(pid_state_t *pid);

/**
 * Reset integrator and derivative state.
 */
void pid_reset(pid_state_t *pid);

/**
 * Set PID gains.
 */
void pid_set_gains(pid_state_t *pid, float kp, float ki, float kd);

/**
 * Compute PID output. Pure function (no side effects beyond updating state).
 *
 * @param pid       PID state (updated in place: integral, prev_error)
 * @param setpoint  Desired value
 * @param measured  Current measured value
 * @param dt        Time delta in seconds
 * @return          Control output
 */
float pid_update(pid_state_t *pid, float setpoint, float measured, float dt);

#endif
