/* SPDX-License-Identifier: Apache-2.0
 *
 * CBMC proof harness for PID controller.
 * Verifies VP-06 (windup clamp) and VP-08 (no overflow).
 *
 * Run: cbmc --unwind 10 -I../../include ../../lib/core/pid.c proof_pid.c
 */

#include <app/core/pid.h>
#include <assert.h>
#include <math.h>

/* VP-06: Integrator is always clamped to [-WINDUP_MAX, +WINDUP_MAX] */
void proof_windup_clamp(void)
{
	pid_state_t pid;
	float setpoint, measured, dt;

	pid_init(&pid);

	/* Nondet gains within reasonable bounds */
	float kp, ki, kd;
	__CPROVER_assume(kp >= 0.0f && kp <= 5.0f);
	__CPROVER_assume(ki >= 0.0f && ki <= 2.0f);
	__CPROVER_assume(kd >= 0.0f && kd <= 1.0f);
	pid_set_gains(&pid, kp, ki, kd);

	/* Run N iterations with arbitrary inputs */
	for (int i = 0; i < 5; i++) {
		__CPROVER_assume(setpoint >= -32768.0f && setpoint <= 32767.0f);
		__CPROVER_assume(measured >= -32768.0f && measured <= 32767.0f);
		__CPROVER_assume(dt >= 1e-6f && dt <= 0.01f);

		float out = pid_update(&pid, setpoint, measured, dt);

		/* VP-06: integrator clamped */
		assert(pid.integral >= -PID_WINDUP_MAX);
		assert(pid.integral <= PID_WINDUP_MAX);

		/* VP-08: output is finite */
		assert(isfinite(out));
	}
}

int main(void)
{
	proof_windup_clamp();
	return 0;
}
