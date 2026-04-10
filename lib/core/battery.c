/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/battery.h>

uint16_t battery_adc_to_mv(uint16_t adc_raw)
{
	/* voltage_mv = (adc_raw * 3300 * 4) / 4095
	 * = adc_raw * 13200 / 4095
	 * Use uint32_t to avoid overflow during multiplication.
	 */
	return (uint16_t)(((uint32_t)adc_raw * 13200U) / 4095U);
}

battery_level_t battery_level(uint16_t voltage_mv, battery_level_t prev_level)
{
	/* Cutoff — no hysteresis, immediate */
	if (voltage_mv < BATT_CUTOFF_MV) {
		return BATT_CUTOFF;
	}

	switch (prev_level) {
	case BATT_NORMAL:
		if (voltage_mv < BATT_CRITICAL_ENTER_MV) {
			return BATT_CRITICAL;
		}
		if (voltage_mv < BATT_WARNING_ENTER_MV) {
			return BATT_WARNING;
		}
		return BATT_NORMAL;

	case BATT_WARNING:
		if (voltage_mv < BATT_CRITICAL_ENTER_MV) {
			return BATT_CRITICAL;
		}
		if (voltage_mv >= BATT_WARNING_EXIT_MV) {
			return BATT_NORMAL;
		}
		return BATT_WARNING;

	case BATT_CRITICAL:
		if (voltage_mv >= BATT_CRITICAL_EXIT_MV) {
			return BATT_WARNING;
		}
		return BATT_CRITICAL;

	case BATT_CUTOFF:
		if (voltage_mv >= BATT_CRITICAL_EXIT_MV) {
			return BATT_WARNING;
		}
		if (voltage_mv >= BATT_CUTOFF_MV) {
			return BATT_CRITICAL;
		}
		return BATT_CUTOFF;

	default:
		return BATT_NORMAL;
	}
}
