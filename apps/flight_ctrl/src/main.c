/* SPDX-License-Identifier: Apache-2.0
 *
 * MiniDrone Flight Controller - Main Loop
 *
 * Orchestrates the effectful shell ↔ pure core data flow per the
 * behavioral spec startup sequence and mode state machine.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

#include <app/shell.h>
#include <app/core/state_machine.h>
#include <app/core/pid.h>
#include <app/core/mixer.h>
#include <app/core/input_mapper.h>
#include <app/core/battery.h>
#include <app/core/calibration.h>
#include <app/core/command.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define PID_LOOP_PERIOD_MS 2    /* 500 Hz */
#define BATTERY_PERIOD_MS  200  /* 5 Hz */
#define TELEM_PERIOD_MS    100  /* 10 Hz */
#define BATT_TELEM_MS      1000 /* 1 Hz */
#define CALIB_SAMPLES      200
#define CALIB_TIMEOUT_MS   5000
#define IMU_TIMEOUT_MS     50
#define MOTOR_RAMP_MS      200
#define MOTOR_TEST_DUTY    1080 /* ~30% of MAX_THROTTLE */
#define MOTOR_TEST_MS      500

/* Debug: string tables for state machine logging */
static const char *mode_str[] = {
	[DRONE_MODE_CALIBRATE] = "CALIBRATE",
	[DRONE_MODE_DIAG]      = "DIAG",
	[DRONE_MODE_RUNNING]   = "RUNNING",
};

static const char *event_str[] = {
	[EVENT_CALIB_OK]         = "CALIB_OK",
	[EVENT_CALIB_FAIL]       = "CALIB_FAIL",
	[EVENT_CALIB_TIMEOUT]    = "CALIB_TIMEOUT",
	[EVENT_ARM]              = "ARM",
	[EVENT_DISARM]           = "DISARM",
	[EVENT_RECALIBRATE]      = "RECALIBRATE",
	[EVENT_BT_DISCONNECT]    = "BT_DISCONNECT",
	[EVENT_LOW_BATT_CRITICAL] = "LOW_BATT_CRITICAL",
	[EVENT_LOW_BATT_CUTOFF]  = "LOW_BATT_CUTOFF",
	[EVENT_IMU_FAILURE]      = "IMU_FAILURE",
	[EVENT_EXCESSIVE_TILT]   = "EXCESSIVE_TILT",
	[EVENT_SW2_PRESS]        = "SW2_PRESS",
};

static const char *action_str[] = {
	[ACTION_NONE]              = "NONE",
	[ACTION_MOTORS_OFF]        = "MOTORS_OFF",
	[ACTION_MOTORS_RAMP_DOWN]  = "MOTORS_RAMP_DOWN",
	[ACTION_PID_RESET]         = "PID_RESET",
	[ACTION_REPORT_CALIB_OK]   = "REPORT_CALIB_OK",
	[ACTION_REPORT_CALIB_FAIL] = "REPORT_CALIB_FAIL",
	[ACTION_REPORT_ERROR]      = "REPORT_ERROR",
	[ACTION_REPORT_MODE]       = "REPORT_MODE",
	[ACTION_REPORT_LOWBATT]    = "REPORT_LOWBATT",
};

static sm_result_t sm_process_logged(const sm_state_t *state,
				     drone_event_t event, uint32_t now_ms)
{
	sm_result_t r = sm_process(state, event, now_ms);

	LOG_INF("SM: %s + %s -> %s (actions:%d)",
		mode_str[state->mode], event_str[event],
		mode_str[r.next_mode], r.action_count);
	for (int i = 0; i < r.action_count; i++) {
		LOG_INF("  action[%d]: %s", i, action_str[r.actions[i]]);
	}
	return r;
}

/* State */
static sm_state_t sm_state;
static pid_state_t pid_pitch, pid_roll, pid_yaw;
static battery_level_t batt_level = BATT_NORMAL;

/* Joystick inputs (updated by BT commands) */
static int16_t throttle_input;
static int16_t yaw_input;
static int16_t pitch_input;
static int16_t roll_input;

/* Ramp-down state */
static bool ramp_active;
static int64_t ramp_start_ms;
static uint16_t ramp_start_duty[4];
static uint16_t last_duty[4];

/* Timers */
static int64_t last_pid_ms;
static int64_t last_batt_ms;
static int64_t last_telem_ms;
static int64_t last_batt_telem_ms;
static int64_t last_imu_ok_ms;

static void execute_actions(const sm_result_t *result)
{
	for (int i = 0; i < result->action_count; i++) {
		switch (result->actions[i]) {
		case ACTION_MOTORS_OFF:
			shell_motor_all_off();
			ramp_active = false;
			memset(last_duty, 0, sizeof(last_duty));
			break;
		case ACTION_MOTORS_RAMP_DOWN:
			ramp_active = true;
			ramp_start_ms = k_uptime_get();
			memcpy(ramp_start_duty, last_duty, sizeof(last_duty));
			break;
		case ACTION_PID_RESET:
			pid_reset(&pid_pitch);
			pid_reset(&pid_roll);
			pid_reset(&pid_yaw);
			throttle_input = 0;
			yaw_input = 0;
			pitch_input = 0;
			roll_input = 0;
			break;
		case ACTION_REPORT_CALIB_OK:
			shell_bt_send_calib(true);
			break;
		case ACTION_REPORT_CALIB_FAIL:
			shell_bt_send_calib(false);
			break;
		case ACTION_REPORT_ERROR:
			shell_bt_send_error("rejected");
			break;
		case ACTION_REPORT_MODE:
			shell_bt_send_mode(result->next_mode);
			break;
		case ACTION_REPORT_LOWBATT:
			shell_bt_send_lowbatt(batt_level);
			break;
		default:
			break;
		}
	}
}

static drone_event_t cmd_to_event(const drone_cmd_t *cmd)
{
	switch (cmd->type) {
	case CMD_ARM:
		return EVENT_ARM;
	case CMD_DISARM:
		return EVENT_DISARM;
	case CMD_CALIBRATE:
		return EVENT_RECALIBRATE;
	default:
		return -1; /* Not a state transition command */
	}
}

static void handle_command(const drone_cmd_t *cmd, uint32_t now_ms)
{
	switch (cmd->type) {
	case CMD_ARM:
	case CMD_DISARM:
	case CMD_CALIBRATE: {
		drone_event_t event = cmd_to_event(cmd);
		sm_result_t result = sm_process_logged(&sm_state, event, now_ms);

		execute_actions(&result);
		sm_apply(&sm_state, &result);
		break;
	}
	case CMD_JOYSTICK:
		if (sm_state.mode != DRONE_MODE_RUNNING) {
			break;
		}
		if (cmd->joystick.axis == 0) {
			throttle_input = cmd->joystick.y;
			yaw_input = cmd->joystick.x;
		} else {
			pitch_input = cmd->joystick.y;
			roll_input = cmd->joystick.x;
		}
		break;
	case CMD_SET_PID_GAIN:
		if (sm_state.mode != DRONE_MODE_RUNNING) {
			break;
		}
		float gain;

		switch (cmd->pid_gain.gain) {
		case GAIN_KP:
			gain = input_map_kp(cmd->pid_gain.value);
			pid_set_gains(&pid_pitch, gain, pid_pitch.ki,
				      pid_pitch.kd);
			pid_set_gains(&pid_roll, gain, pid_roll.ki,
				      pid_roll.kd);
			pid_set_gains(&pid_yaw, gain, pid_yaw.ki, pid_yaw.kd);
			break;
		case GAIN_KI:
			gain = input_map_ki(cmd->pid_gain.value);
			pid_set_gains(&pid_pitch, pid_pitch.kp, gain,
				      pid_pitch.kd);
			pid_set_gains(&pid_roll, pid_roll.kp, gain,
				      pid_roll.kd);
			pid_set_gains(&pid_yaw, pid_yaw.kp, gain, pid_yaw.kd);
			break;
		case GAIN_KD:
			gain = input_map_kd(cmd->pid_gain.value);
			pid_set_gains(&pid_pitch, pid_pitch.kp, pid_pitch.ki,
				      gain);
			pid_set_gains(&pid_roll, pid_roll.kp, pid_roll.ki,
				      gain);
			pid_set_gains(&pid_yaw, pid_yaw.kp, pid_yaw.ki, gain);
			break;
		}
		break;
	case CMD_MOTOR_TEST:
		if (sm_state.mode != DRONE_MODE_DIAG) {
			break;
		}
		if (cmd->motor_test.motor_id < 4) {
			uint16_t test_duty[4] = {0};

			test_duty[cmd->motor_test.motor_id] = MOTOR_TEST_DUTY;
			shell_motor_set(test_duty);
			k_msleep(MOTOR_TEST_MS);
			shell_motor_all_off();
		}
		break;
	default:
		break;
	}
}

static void run_calibration(void)
{
	LOG_INF("Starting IMU calibration...");
	shell_led_set(0, false); /* Will blink in main loop */

	/* Discard initial samples while MPU6050 settles after power-on */
	int16_t discard[6];

	for (int i = 0; i < 50; i++) {
		shell_imu_read(discard);
		k_msleep(5);
	}

	static int16_t samples[CALIB_SAMPLES][CALIB_AXES];
	int64_t start = k_uptime_get();

	for (int i = 0; i < CALIB_SAMPLES; i++) {
		if ((k_uptime_get() - start) > CALIB_TIMEOUT_MS) {
			sm_result_t r = sm_process_logged(&sm_state,
						   EVENT_CALIB_TIMEOUT,
						   (uint32_t)k_uptime_get());
			execute_actions(&r);
			sm_apply(&sm_state, &r);
			return;
		}

		int ret = shell_imu_read(samples[i]);

		if (ret < 0) {
			sm_result_t r = sm_process_logged(&sm_state, EVENT_CALIB_FAIL,
						   (uint32_t)k_uptime_get());
			execute_actions(&r);
			sm_apply(&sm_state, &r);
			return;
		}
		k_msleep(5);
	}

	/* Debug: log first few samples */
	LOG_INF("Sample[0]: [%d,%d,%d,%d,%d,%d]",
		samples[0][0], samples[0][1], samples[0][2],
		samples[0][3], samples[0][4], samples[0][5]);
	LOG_INF("Sample[99]: [%d,%d,%d,%d,%d,%d]",
		samples[99][0], samples[99][1], samples[99][2],
		samples[99][3], samples[99][4], samples[99][5]);
	LOG_INF("Sample[199]: [%d,%d,%d,%d,%d,%d]",
		samples[199][0], samples[199][1], samples[199][2],
		samples[199][3], samples[199][4], samples[199][5]);

	/* Debug: compute and log per-axis variance */
	for (int a = 0; a < CALIB_AXES; a++) {
		int64_t sum = 0;
		for (int i = 0; i < CALIB_SAMPLES; i++) {
			sum += samples[i][a];
		}
		int16_t mean = (int16_t)(sum / CALIB_SAMPLES);
		int64_t var_sum = 0;
		for (int i = 0; i < CALIB_SAMPLES; i++) {
			int32_t diff = samples[i][a] - mean;
			var_sum += (int64_t)diff * diff;
		}
		int64_t variance = var_sum / CALIB_SAMPLES;
		LOG_INF("Axis %d: mean=%d variance=%lld", a, mean,
			variance);
	}

	calib_result_t cr = calib_compute(samples, CALIB_SAMPLES);
	drone_event_t event = cr.ok ? EVENT_CALIB_OK : EVENT_CALIB_FAIL;
	sm_result_t r = sm_process_logged(&sm_state, event,
				   (uint32_t)k_uptime_get());

	execute_actions(&r);
	sm_apply(&sm_state, &r);

	if (cr.ok) {
		LOG_INF("Calibration OK: offsets [%d,%d,%d,%d,%d,%d]",
			cr.offsets[0], cr.offsets[1], cr.offsets[2],
			cr.offsets[3], cr.offsets[4], cr.offsets[5]);
	} else {
		LOG_WRN("Calibration failed: high variance (threshold=%d)",
			CALIB_VARIANCE_THRESHOLD);
	}
}

static void run_pid_loop(int64_t now_ms)
{
	float dt = (float)(now_ms - last_pid_ms) / 1000.0f;

	last_pid_ms = now_ms;

	/* Read IMU */
	int16_t imu_data[6];

	if (shell_imu_read(imu_data) < 0) {
		if ((now_ms - last_imu_ok_ms) > IMU_TIMEOUT_MS) {
			sm_result_t r = sm_process_logged(&sm_state,
						   EVENT_IMU_FAILURE,
						   (uint32_t)now_ms);
			execute_actions(&r);
			sm_apply(&sm_state, &r);
		}
		return;
	}
	last_imu_ok_ms = now_ms;

	/* Map throttle from signed input to [0, MAX_THROTTLE] */
	float thr = 0.0f;

	if (throttle_input > 0) {
		thr = (float)throttle_input * (float)MAX_THROTTLE / 512.0f;
	}

	/* PID update */
	float p_out = pid_update(&pid_pitch, (float)pitch_input,
				 (float)imu_data[0], dt);
	float r_out = pid_update(&pid_roll, (float)roll_input,
				 (float)imu_data[1], dt);
	float y_out = pid_update(&pid_yaw, (float)yaw_input,
				 (float)imu_data[5], dt);

	/* Mix and output */
	mixer_update(thr, p_out, r_out, y_out, last_duty);
	shell_motor_set(last_duty);
}

static void update_ramp(int64_t now_ms)
{
	if (!ramp_active) {
		return;
	}

	int64_t elapsed = now_ms - ramp_start_ms;

	if (elapsed >= MOTOR_RAMP_MS) {
		shell_motor_all_off();
		ramp_active = false;
		memset(last_duty, 0, sizeof(last_duty));
		return;
	}

	float factor = 1.0f - (float)elapsed / (float)MOTOR_RAMP_MS;
	uint16_t duty[4];

	for (int i = 0; i < 4; i++) {
		duty[i] = (uint16_t)((float)ramp_start_duty[i] * factor);
	}
	shell_motor_set(duty);
}

int main(void)
{
	LOG_INF("MiniDrone Flight Controller starting...");

	/* Step 2-6: Initialize all subsystems */
	int ret;

	ret = shell_led_init();
	if (ret < 0) {
		LOG_ERR("LED init failed: %d", ret);
	}

	shell_led_set(0, true); /* STAT_0 on during init */

	ret = shell_bt_init();
	if (ret < 0) {
		LOG_ERR("Bluetooth init failed: %d", ret);
	}

	ret = shell_motor_init();
	if (ret < 0) {
		LOG_ERR("Motor init failed: %d", ret);
	}
	shell_motor_all_off();

	ret = shell_battery_init();
	if (ret < 0) {
		LOG_ERR("Battery init failed: %d", ret);
	}

	/* Step 7: Verify IMU */
	ret = shell_imu_init();
	if (ret < 0) {
		LOG_ERR("IMU init failed: %d", ret);
		shell_bt_send_error("imu_init_fail");
		/* Watchdog will reset us */
		while (1) {
			shell_led_toggle(0);
			k_msleep(100);
		}
	}

	/* Initialize pure core state */
	sm_init(&sm_state);
	pid_init(&pid_pitch);
	pid_init(&pid_roll);
	pid_init(&pid_yaw);

	/* Set default PID gains */
	pid_set_gains(&pid_pitch, 1.0f, 0.1f, 0.05f);
	pid_set_gains(&pid_roll, 1.0f, 0.1f, 0.05f);
	pid_set_gains(&pid_yaw, 1.0f, 0.1f, 0.05f);

	int64_t now_ms = k_uptime_get();

	last_pid_ms = now_ms;
	last_batt_ms = now_ms;
	last_telem_ms = now_ms;
	last_batt_telem_ms = now_ms;
	last_imu_ok_ms = now_ms;

	/* Log initial battery reading */
	{
		uint16_t raw = shell_battery_read_raw();
		uint16_t mv = battery_adc_to_mv(raw);

		LOG_INF("Battery: raw=%u mv=%u", raw, mv);
	}

	LOG_INF("All systems initialized. Entering Calibrate mode.");

	/* Main loop */
	while (1) {
		now_ms = k_uptime_get();

		/* Poll BT commands */
		drone_cmd_t cmd = shell_bt_poll();

		if (cmd.type != CMD_NONE) {
			handle_command(&cmd, (uint32_t)now_ms);
		}

		/* Battery monitoring (5 Hz) */
		if ((now_ms - last_batt_ms) >= BATTERY_PERIOD_MS) {
			last_batt_ms = now_ms;
			uint16_t raw = shell_battery_read_raw();
			uint16_t mv = battery_adc_to_mv(raw);
			battery_level_t new_level = battery_level(mv,
								  batt_level);

			if (new_level != batt_level) {
				batt_level = new_level;
				if (batt_level == BATT_CUTOFF) {
					sm_result_t r = sm_process_logged(
						&sm_state,
						EVENT_LOW_BATT_CUTOFF,
						(uint32_t)now_ms);
					execute_actions(&r);
					sm_apply(&sm_state, &r);
				} else if (batt_level == BATT_CRITICAL) {
					sm_result_t r = sm_process_logged(
						&sm_state,
						EVENT_LOW_BATT_CRITICAL,
						(uint32_t)now_ms);
					execute_actions(&r);
					sm_apply(&sm_state, &r);
				}
				shell_led_set(2, batt_level >= BATT_WARNING);
			}

			/* Battery telemetry (1 Hz) */
			if ((now_ms - last_batt_telem_ms) >= BATT_TELEM_MS) {
				last_batt_telem_ms = now_ms;
				shell_bt_send_battery(mv);
			}
		}

		/* Mode-specific logic */
		switch (sm_state.mode) {
		case DRONE_MODE_CALIBRATE:
			run_calibration();
			break;

		case DRONE_MODE_DIAG:
			shell_led_set(0, true);
			shell_led_set(1, false);
			/* IMU telemetry (10 Hz) */
			if ((now_ms - last_telem_ms) >= TELEM_PERIOD_MS) {
				last_telem_ms = now_ms;
				int16_t imu_data[6];

				if (shell_imu_read(imu_data) == 0) {
					shell_bt_send_imu(imu_data);
				}
			}
			break;

		case DRONE_MODE_RUNNING:
			shell_led_set(0, true);
			shell_led_set(1, true);
			if (ramp_active) {
				update_ramp(now_ms);
			} else if ((now_ms - last_pid_ms) >= PID_LOOP_PERIOD_MS) {
				run_pid_loop(now_ms);
			}
			break;
		}

		k_msleep(1);
	}

	return 0;
}
