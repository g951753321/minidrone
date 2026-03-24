/* SPDX-License-Identifier: Apache-2.0
 *
 * CBMC proof harness for battery monitor.
 * Verifies VP-10 (no overflow) and VP-18 (hysteresis correctness).
 *
 * Run: cbmc -I../../include ../../lib/core/battery.c proof_battery.c
 */

#include <app/core/battery.h>
#include <assert.h>

/* VP-10: ADC-to-mV conversion never overflows uint16 */
void proof_adc_no_overflow(void)
{
	uint16_t adc_raw;

	/* All possible 12-bit ADC values */
	__CPROVER_assume(adc_raw <= 4095);

	uint16_t mv = battery_adc_to_mv(adc_raw);

	/* Result must be a reasonable voltage (< 10V = 10000 mV) */
	assert(mv <= 10000);
}

/* VP-18: Hysteresis — warning entry/exit thresholds */
void proof_hysteresis_warning(void)
{
	uint16_t voltage_mv;
	battery_level_t prev;

	__CPROVER_assume(voltage_mv >= 5000 && voltage_mv <= 9000);
	__CPROVER_assume(prev >= BATT_NORMAL && prev <= BATT_CUTOFF);

	battery_level_t level = battery_level(voltage_mv, prev);

	/* Result must be a valid enum */
	assert(level >= BATT_NORMAL && level <= BATT_CUTOFF);

	/* Hysteresis: if NORMAL and voltage >= WARNING_EXIT, stay NORMAL */
	if (prev == BATT_NORMAL && voltage_mv >= BATT_WARNING_EXIT_MV) {
		assert(level == BATT_NORMAL);
	}

	/* Hysteresis: if WARNING and voltage between enter/exit, stay WARNING */
	if (prev == BATT_WARNING &&
	    voltage_mv >= BATT_WARNING_ENTER_MV &&
	    voltage_mv < BATT_WARNING_EXIT_MV) {
		assert(level == BATT_WARNING);
	}

	/* Cutoff always applies below threshold */
	if (voltage_mv < BATT_CUTOFF_MV) {
		assert(level == BATT_CUTOFF);
	}
}

int main(void)
{
	proof_adc_no_overflow();
	proof_hysteresis_warning();
	return 0;
}
