/* SPDX-License-Identifier: Apache-2.0 */

#include <app/shell.h>
#include <zephyr/drivers/sensor.h>

static const struct device *imu_dev;

int shell_imu_init(void)
{
	imu_dev = DEVICE_DT_GET(DT_NODELABEL(mpu6050));
	if (!device_is_ready(imu_dev)) {
		return -ENODEV;
	}
	return 0;
}

int shell_imu_read(int16_t data[6])
{
	struct sensor_value accel[3], gyro[3];
	int ret;

	ret = sensor_sample_fetch(imu_dev);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	if (ret < 0) {
		return ret;
	}

	/* Convert sensor_value (val1.val2) to raw int16 scale.
	 * Accel: sensor returns m/s^2, scale to ~raw register equivalent.
	 * Gyro: sensor returns rad/s.
	 * We store as fixed-point: val1 * 1000 + val2 / 1000.
	 */
	for (int i = 0; i < 3; i++) {
		data[i] = (int16_t)(accel[i].val1 * 1000 +
				    accel[i].val2 / 1000);
		data[3 + i] = (int16_t)(gyro[i].val1 * 1000 +
					gyro[i].val2 / 1000);
	}

	return 0;
}
