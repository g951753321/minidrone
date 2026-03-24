/* SPDX-License-Identifier: Apache-2.0 */

#include <app/core/calibration.h>
#include <string.h>

calib_result_t calib_compute(const int16_t (*samples)[CALIB_AXES],
			     uint16_t sample_count)
{
	calib_result_t result;
	memset(&result, 0, sizeof(result));
	result.ok = false;

	if (sample_count == 0 || samples == NULL) {
		return result;
	}

	/* Compute mean for each axis */
	int32_t sum[CALIB_AXES] = {0};
	for (uint16_t i = 0; i < sample_count; i++) {
		for (int a = 0; a < CALIB_AXES; a++) {
			sum[a] += samples[i][a];
		}
	}
	for (int a = 0; a < CALIB_AXES; a++) {
		result.offsets[a] = (int16_t)(sum[a] / sample_count);
	}

	/* Compute variance for each axis and check threshold */
	result.ok = true;
	for (int a = 0; a < CALIB_AXES; a++) {
		int32_t variance_sum = 0;
		for (uint16_t i = 0; i < sample_count; i++) {
			int32_t diff = samples[i][a] - result.offsets[a];
			variance_sum += diff * diff;
		}
		int32_t variance = variance_sum / sample_count;
		if (variance > CALIB_VARIANCE_THRESHOLD) {
			result.ok = false;
			break;
		}
	}

	return result;
}
