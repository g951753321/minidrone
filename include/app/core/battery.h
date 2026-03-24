/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_BATTERY_H
#define APP_CORE_BATTERY_H

#include <stdint.h>

typedef enum {
	BATT_NORMAL,
	BATT_WARNING,
	BATT_CRITICAL,
	BATT_CUTOFF,
} battery_level_t;

/* Threshold voltages in mV (pack voltage) */
#define BATT_WARNING_ENTER_MV   7000
#define BATT_WARNING_EXIT_MV    7200
#define BATT_CRITICAL_ENTER_MV  6600
#define BATT_CRITICAL_EXIT_MV   6800
#define BATT_CUTOFF_MV          6000

/**
 * Convert raw ADC value to pack voltage in mV.
 * Voltage divider: R_top=30K, R_bottom=20K, ratio=0.4
 * Formula: voltage_mv = (adc_raw * 3300 * 5) / (4095 * 2)
 * Pure function.
 *
 * @param adc_raw  12-bit ADC reading (0-4095)
 * @return         Pack voltage in millivolts
 */
uint16_t battery_adc_to_mv(uint16_t adc_raw);

/**
 * Determine battery level with hysteresis.
 * Pure function.
 *
 * @param voltage_mv  Current pack voltage in mV
 * @param prev_level  Previous battery level (for hysteresis)
 * @return            New battery level
 */
battery_level_t battery_level(uint16_t voltage_mv, battery_level_t prev_level);

#endif
