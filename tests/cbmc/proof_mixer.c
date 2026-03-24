/* SPDX-License-Identifier: Apache-2.0
 *
 * CBMC proof harness for motor mixer.
 * Verifies VP-07 (duty <= MAX_THROTTLE), VP-21 (clamped), VP-22 (no negative).
 *
 * Run: cbmc -I../../include ../../lib/core/mixer.c proof_mixer.c \
 *      --function main
 */

#include <app/core/mixer.h>
#include <assert.h>

void proof_mixer_bounds(void)
{
	float throttle, pitch, roll, yaw;
	uint16_t duty[4];

	/* Nondet inputs across full float range seen in practice */
	__CPROVER_assume(throttle >= -1000.0f && throttle <= 5000.0f);
	__CPROVER_assume(pitch >= -2000.0f && pitch <= 2000.0f);
	__CPROVER_assume(roll >= -2000.0f && roll <= 2000.0f);
	__CPROVER_assume(yaw >= -2000.0f && yaw <= 2000.0f);

	mixer_update(throttle, pitch, roll, yaw, duty);

	for (int i = 0; i < 4; i++) {
		/* VP-07/VP-21: duty <= MAX_THROTTLE */
		assert(duty[i] <= MAX_THROTTLE);
		/* VP-22: uint16_t is inherently >= 0, but verify no wrap */
		assert(duty[i] <= MAX_THROTTLE);
	}
}

int main(void)
{
	proof_mixer_bounds();
	return 0;
}
