/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/pid.h>
#include <string.h>

void pid_init(pid_state_t *pid)
{
	memset(pid, 0, sizeof(*pid));
	pid->initialized = false;
}

void pid_reset(pid_state_t *pid)
{
	pid->integral = 0.0f;
	pid->prev_error = 0.0f;
	pid->initialized = false;
}

void pid_set_gains(pid_state_t *pid, float kp, float ki, float kd)
{
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
}

float pid_update(pid_state_t *pid, float setpoint, float measured, float dt)
{
	float error = setpoint - measured;

	/* Proportional */
	float p_out = pid->kp * error;

	/* Integral with anti-windup clamp */
	pid->integral += error * dt;
	if (pid->integral > PID_WINDUP_MAX) {
		pid->integral = PID_WINDUP_MAX;
	} else if (pid->integral < -PID_WINDUP_MAX) {
		pid->integral = -PID_WINDUP_MAX;
	}
	float i_out = pid->ki * pid->integral;

	/* Derivative (skip on first call) */
	float d_out = 0.0f;
	if (pid->initialized && dt > 0.0f) {
		d_out = pid->kd * (error - pid->prev_error) / dt;
	}

	pid->prev_error = error;
	pid->initialized = true;

	return p_out + i_out + d_out;
}
