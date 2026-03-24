/* SPDX-License-Identifier: Apache-2.0 */

#ifndef APP_CORE_CALIBRATION_H
#define APP_CORE_CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>

#define CALIB_AXES 6
#define CALIB_VARIANCE_THRESHOLD 100

typedef struct {
	int16_t offsets[CALIB_AXES];
	bool ok;
} calib_result_t;

/**
 * Compute IMU calibration offsets from raw samples.
 * Returns average offset per axis and checks variance.
 * Pure function.
 *
 * @param samples       Array of raw samples, each with 6 int16 values
 *                      [ax, ay, az, gx, gy, gz]
 * @param sample_count  Number of samples (rows)
 * @return              Result with offsets and ok flag
 */
calib_result_t calib_compute(const int16_t (*samples)[CALIB_AXES],
			     uint16_t sample_count);

#endif
