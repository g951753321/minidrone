/* SPDX-License-Identifier: Apache-2.0
 *
 * VSDD Phase 2a: Test Suite for Pure Core Modules
 *
 * All tests are written BEFORE implementation.
 * Every test must FAIL until the corresponding module is implemented.
 *
 * Traceability: each test references a VP/VT/EC ID from verification.md
 */

#include <zephyr/ztest.h>
#include <math.h>
#include <string.h>

#include <app/core/state_machine.h>
#include <app/core/pid.h>
#include <app/core/mixer.h>
#include <app/core/input_mapper.h>
#include <app/core/battery.h>
#include <app/core/microblue_parser.h>
#include <app/core/calibration.h>

/* ===================================================================
 * State Machine Tests (VP-01 through VP-05, VP-11, VP-20)
 * =================================================================== */

ZTEST_SUITE(state_machine, NULL, NULL, NULL, NULL, NULL);

/* VP-01: State machine never reaches an undefined state */
ZTEST(state_machine, test_init_starts_in_calibrate)
{
	sm_state_t state;

	sm_init(&state);
	zassert_equal(state.mode, DRONE_MODE_CALIBRATE);
	zassert_false(state.calibration_valid);
}

/* VP-03: Calibrate → Diag only on success */
ZTEST(state_machine, test_calib_ok_transitions_to_diag)
{
	sm_state_t state;

	sm_init(&state);
	sm_result_t r = sm_process(&state, EVENT_CALIB_OK, 0);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
}

/* VP-03: Calibrate never transitions directly to Running */
ZTEST(state_machine, test_calib_arm_ignored)
{
	sm_state_t state;

	sm_init(&state);
	sm_result_t r = sm_process(&state, EVENT_ARM, 0);

	zassert_equal(r.next_mode, DRONE_MODE_CALIBRATE,
		      "ARM in Calibrate must be ignored");
}

/* VP-03: Calibrate fail stays in Calibrate */
ZTEST(state_machine, test_calib_fail_stays)
{
	sm_state_t state;

	sm_init(&state);
	sm_result_t r = sm_process(&state, EVENT_CALIB_FAIL, 0);

	zassert_equal(r.next_mode, DRONE_MODE_CALIBRATE);
}

/* VP-11: Calibration timeout exits Calibrate */
ZTEST(state_machine, test_calib_timeout_stays)
{
	sm_state_t state;

	sm_init(&state);
	sm_result_t r = sm_process(&state, EVENT_CALIB_TIMEOUT, 0);

	zassert_equal(r.next_mode, DRONE_MODE_CALIBRATE);
}

/* Diag → Running on arm (with valid calibration) */
ZTEST(state_machine, test_diag_arm_transitions_to_running)
{
	sm_state_t state = {
		.mode = DRONE_MODE_DIAG,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_ARM, 2000);

	zassert_equal(r.next_mode, DRONE_MODE_RUNNING);
}

/* EC-31: Arm rejected when calibration invalid */
ZTEST(state_machine, test_diag_arm_rejected_no_calibration)
{
	sm_state_t state = {
		.mode = DRONE_MODE_DIAG,
		.calibration_valid = false,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_ARM, 2000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG,
		      "ARM must be rejected without valid calibration");
}

/* VP-20: Arm debounce rejects re-arm within 1s of disarm */
ZTEST(state_machine, test_diag_arm_debounce)
{
	sm_state_t state = {
		.mode = DRONE_MODE_DIAG,
		.calibration_valid = true,
		.last_disarm_ms = 1000,
	};

	/* Try to arm 500ms after last disarm — should be rejected */
	sm_result_t r = sm_process(&state, EVENT_ARM, 1500);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG,
		      "ARM within 1s of disarm must be rejected");
}

/* VP-04: Disarm from Running reaches Diag */
ZTEST(state_machine, test_running_disarm_to_diag)
{
	sm_state_t state = {
		.mode = DRONE_MODE_RUNNING,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_DISARM, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
}

/* VP-02: Motors off on BT disconnect (Running → Diag) */
ZTEST(state_machine, test_running_bt_disconnect)
{
	sm_state_t state = {
		.mode = DRONE_MODE_RUNNING,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_BT_DISCONNECT, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
	/* Must include motor ramp down action */
	bool has_ramp = false;

	for (int i = 0; i < r.action_count; i++) {
		if (r.actions[i] == ACTION_MOTORS_RAMP_DOWN) {
			has_ramp = true;
		}
	}
	zassert_true(has_ramp, "BT disconnect must trigger motor ramp down");
}

/* VP-05: Battery cutoff forces motors off in all states */
ZTEST(state_machine, test_cutoff_in_running)
{
	sm_state_t state = {
		.mode = DRONE_MODE_RUNNING,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_LOW_BATT_CUTOFF, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
	bool has_off = false;

	for (int i = 0; i < r.action_count; i++) {
		if (r.actions[i] == ACTION_MOTORS_OFF) {
			has_off = true;
		}
	}
	zassert_true(has_off, "Cutoff must force motors off immediately");
}

/* VP-02: BT disconnect in Diag stays in Diag (motors already off) */
ZTEST(state_machine, test_diag_bt_disconnect_stays)
{
	sm_state_t state = {
		.mode = DRONE_MODE_DIAG,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_BT_DISCONNECT, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
}

/* EC-28: Excessive tilt in Running */
ZTEST(state_machine, test_running_excessive_tilt)
{
	sm_state_t state = {
		.mode = DRONE_MODE_RUNNING,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_EXCESSIVE_TILT, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
	bool has_off = false;

	for (int i = 0; i < r.action_count; i++) {
		if (r.actions[i] == ACTION_MOTORS_OFF) {
			has_off = true;
		}
	}
	zassert_true(has_off, "Excessive tilt must cut motors immediately");
}

/* Recalibrate from Diag */
ZTEST(state_machine, test_diag_recalibrate)
{
	sm_state_t state = {
		.mode = DRONE_MODE_DIAG,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_RECALIBRATE, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_CALIBRATE);
}

/* EC-34: SW2 press in Running disarms */
ZTEST(state_machine, test_running_sw2_disarms)
{
	sm_state_t state = {
		.mode = DRONE_MODE_RUNNING,
		.calibration_valid = true,
		.last_disarm_ms = 0,
	};

	sm_result_t r = sm_process(&state, EVENT_SW2_PRESS, 5000);

	zassert_equal(r.next_mode, DRONE_MODE_DIAG);
}

/* VP-01: All events in all modes produce valid states */
ZTEST(state_machine, test_all_transitions_produce_valid_state)
{
	drone_mode_t modes[] = {DRONE_MODE_CALIBRATE, DRONE_MODE_DIAG,
				DRONE_MODE_RUNNING};
	drone_event_t events[] = {EVENT_CALIB_OK,	  EVENT_CALIB_FAIL,
				  EVENT_CALIB_TIMEOUT,	  EVENT_ARM,
				  EVENT_DISARM,		  EVENT_RECALIBRATE,
				  EVENT_BT_DISCONNECT,	  EVENT_LOW_BATT_CRITICAL,
				  EVENT_LOW_BATT_CUTOFF,  EVENT_IMU_FAILURE,
				  EVENT_EXCESSIVE_TILT,	  EVENT_SW2_PRESS};

	for (int m = 0; m < 3; m++) {
		for (int e = 0; e < 12; e++) {
			sm_state_t state = {
				.mode = modes[m],
				.calibration_valid = true,
				.last_disarm_ms = 0,
			};
			sm_result_t r = sm_process(&state, events[e], 5000);

			zassert_true(r.next_mode >= DRONE_MODE_CALIBRATE &&
					     r.next_mode <= DRONE_MODE_RUNNING,
				     "Invalid state from mode=%d event=%d", m,
				     e);
		}
	}
}

/* ===================================================================
 * PID Controller Tests (VP-06, VP-08, VP-09)
 * =================================================================== */

ZTEST_SUITE(pid, NULL, NULL, NULL, NULL, NULL);

/* Basic: zero error produces zero output */
ZTEST(pid, test_zero_error_zero_output)
{
	pid_state_t pid;

	pid_init(&pid);
	pid_set_gains(&pid, 1.0f, 0.0f, 0.0f);

	float out = pid_update(&pid, 100.0f, 100.0f, 0.002f);

	zassert_true(fabsf(out) < 0.001f, "Zero error must produce zero output");
}

/* P-only: output proportional to error */
ZTEST(pid, test_p_only)
{
	pid_state_t pid;

	pid_init(&pid);
	pid_set_gains(&pid, 2.0f, 0.0f, 0.0f);

	float out = pid_update(&pid, 10.0f, 5.0f, 0.002f);

	zassert_true(fabsf(out - 10.0f) < 0.001f, "Kp=2, error=5, expect 10");
}

/* VP-06: Integrator clamped to WINDUP_MAX */
ZTEST(pid, test_integrator_windup_clamp)
{
	pid_state_t pid;

	pid_init(&pid);
	pid_set_gains(&pid, 0.0f, 1.0f, 0.0f);

	/* Drive integrator with large error for many iterations */
	for (int i = 0; i < 100000; i++) {
		pid_update(&pid, 1000.0f, 0.0f, 0.002f);
	}

	zassert_true(fabsf(pid.integral) <= PID_WINDUP_MAX,
		     "Integrator must be clamped to WINDUP_MAX");
}

/* VP-08: No overflow with extreme int16 inputs */
ZTEST(pid, test_no_overflow_extreme_inputs)
{
	pid_state_t pid;

	pid_init(&pid);
	pid_set_gains(&pid, 5.0f, 2.0f, 1.0f);

	float out = pid_update(&pid, 32767.0f, -32768.0f, 0.002f);

	zassert_true(isfinite(out), "Output must be finite for extreme inputs");
}

/* Reset clears state */
ZTEST(pid, test_reset_clears_state)
{
	pid_state_t pid;

	pid_init(&pid);
	pid_set_gains(&pid, 1.0f, 1.0f, 1.0f);
	pid_update(&pid, 100.0f, 0.0f, 0.002f);
	pid_reset(&pid);

	zassert_true(fabsf(pid.integral) < 0.001f);
	zassert_true(fabsf(pid.prev_error) < 0.001f);
}

/* ===================================================================
 * Motor Mixer Tests (VP-07, VP-21, VP-22)
 * =================================================================== */

ZTEST_SUITE(mixer, NULL, NULL, NULL, NULL, NULL);

/* VP-21: All outputs clamped to [0, MAX_THROTTLE] */
ZTEST(mixer, test_output_clamped_max)
{
	uint16_t duty[4];

	mixer_update(MAX_THROTTLE, 500.0f, 500.0f, 500.0f, duty);

	for (int i = 0; i < 4; i++) {
		zassert_true(duty[i] <= MAX_THROTTLE,
			     "Motor %d duty %u exceeds MAX_THROTTLE", i,
			     duty[i]);
	}
}

/* VP-22: No negative duty values */
ZTEST(mixer, test_output_no_negative)
{
	uint16_t duty[4];

	/* Large negative PID outputs should clamp to 0, not wrap */
	mixer_update(100.0f, -2000.0f, -2000.0f, -2000.0f, duty);

	for (int i = 0; i < 4; i++) {
		/* uint16 can't be negative, but verify no wrap-around */
		zassert_true(duty[i] <= MAX_THROTTLE,
			     "Motor %d duty %u wrapped around", i, duty[i]);
	}
}

/* Zero throttle, zero PID → all motors at 0 */
ZTEST(mixer, test_zero_throttle_zero_pid)
{
	uint16_t duty[4];

	mixer_update(0.0f, 0.0f, 0.0f, 0.0f, duty);

	for (int i = 0; i < 4; i++) {
		zassert_equal(duty[i], 0, "Motor %d should be 0", i);
	}
}

/* Hover: equal throttle, no PID → all motors equal */
ZTEST(mixer, test_hover_equal_motors)
{
	uint16_t duty[4];

	mixer_update(1800.0f, 0.0f, 0.0f, 0.0f, duty);

	zassert_equal(duty[MOTOR_FR], duty[MOTOR_FL]);
	zassert_equal(duty[MOTOR_FL], duty[MOTOR_BR]);
	zassert_equal(duty[MOTOR_BR], duty[MOTOR_BL]);
	zassert_equal(duty[MOTOR_FR], 1800);
}

/* Pitch forward: front motors higher than back */
ZTEST(mixer, test_pitch_forward)
{
	uint16_t duty[4];

	mixer_update(1800.0f, 200.0f, 0.0f, 0.0f, duty);

	zassert_true(duty[MOTOR_FR] > duty[MOTOR_BR],
		     "Pitch forward: FR > BR");
	zassert_true(duty[MOTOR_FL] > duty[MOTOR_BL],
		     "Pitch forward: FL > BL");
}

/* Roll right: right motors higher than left */
ZTEST(mixer, test_roll_right)
{
	uint16_t duty[4];

	mixer_update(1800.0f, 0.0f, 200.0f, 0.0f, duty);

	zassert_true(duty[MOTOR_FR] > duty[MOTOR_FL], "Roll right: FR > FL");
	zassert_true(duty[MOTOR_BR] > duty[MOTOR_BL], "Roll right: BR > BL");
}

/* ===================================================================
 * Input Mapper Tests (VP-09, VP-23, EC-10)
 * =================================================================== */

ZTEST_SUITE(input_mapper, NULL, NULL, NULL, NULL, NULL);

/* VP-09: Joystick center produces exactly zero */
ZTEST(input_mapper, test_center_produces_zero)
{
	int16_t result = input_map_joystick(512);

	zassert_equal(result, 0, "Center (512) must produce 0, got %d", result);
}

/* Min and max */
ZTEST(input_mapper, test_joystick_min_max)
{
	int16_t min_val = input_map_joystick(0);
	int16_t max_val = input_map_joystick(1023);

	zassert_equal(min_val, -512);
	zassert_equal(max_val, 511);
}

/* VP-23: Slider Kp mapping range */
ZTEST(input_mapper, test_kp_range)
{
	float kp_min = input_map_kp(0);
	float kp_max = input_map_kp(100);

	zassert_true(fabsf(kp_min) < 0.001f, "Kp(0) must be 0.0");
	zassert_true(fabsf(kp_max - 5.0f) < 0.001f, "Kp(100) must be 5.0");
}

/* VP-23: Slider Ki mapping range */
ZTEST(input_mapper, test_ki_range)
{
	float ki_min = input_map_ki(0);
	float ki_max = input_map_ki(100);

	zassert_true(fabsf(ki_min) < 0.001f, "Ki(0) must be 0.0");
	zassert_true(fabsf(ki_max - 2.0f) < 0.001f, "Ki(100) must be 2.0");
}

/* VP-23: Slider Kd mapping range */
ZTEST(input_mapper, test_kd_range)
{
	float kd_min = input_map_kd(0);
	float kd_max = input_map_kd(100);

	zassert_true(fabsf(kd_min) < 0.001f, "Kd(0) must be 0.0");
	zassert_true(fabsf(kd_max - 1.0f) < 0.001f, "Kd(100) must be 1.0");
}

/* EC-10: Slider value clamped (input > 100 should still produce valid gain) */
ZTEST(input_mapper, test_kp_out_of_range_clamped)
{
	float kp = input_map_kp(200);

	zassert_true(kp <= 5.0f, "Kp must be clamped to max 5.0");
}

/* ===================================================================
 * Battery Monitor Tests (VP-10, VP-18, EC-21)
 * =================================================================== */

ZTEST_SUITE(battery, NULL, NULL, NULL, NULL, NULL);

/* VP-10: ADC to mV conversion for known values */
ZTEST(battery, test_adc_to_mv_full_charge)
{
	/* Full charge 8.4V → ADC sees 3.36V → adc_raw ≈ 4095 * 3.36/3.3 ≈ 4169
	 * but ADC clamps at 4095. At 4095: voltage = 4095*3300*5/(4095*2) = 8250 mV
	 */
	uint16_t mv = battery_adc_to_mv(4095);

	zassert_true(mv > 8000 && mv < 8500, "Full ADC should be ~8250mV, got %u", mv);
}

/* VP-10: ADC conversion never overflows uint16 */
ZTEST(battery, test_adc_to_mv_no_overflow)
{
	/* Even with max ADC value, result should fit uint16 (max 65535) */
	uint16_t mv = battery_adc_to_mv(4095);

	zassert_true(mv < 10000, "Conversion should not overflow, got %u", mv);
}

/* Zero ADC → zero voltage */
ZTEST(battery, test_adc_to_mv_zero)
{
	uint16_t mv = battery_adc_to_mv(0);

	zassert_equal(mv, 0);
}

/* VP-18: Battery hysteresis — enter warning at 7.0V */
ZTEST(battery, test_hysteresis_enter_warning)
{
	battery_level_t level = battery_level(6900, BATT_NORMAL);

	zassert_equal(level, BATT_WARNING);
}

/* VP-18: Battery hysteresis — exit warning at 7.2V */
ZTEST(battery, test_hysteresis_exit_warning)
{
	battery_level_t level = battery_level(7300, BATT_WARNING);

	zassert_equal(level, BATT_NORMAL);
}

/* VP-18: Stay in warning between thresholds (7.0V - 7.2V) */
ZTEST(battery, test_hysteresis_stay_warning)
{
	battery_level_t level = battery_level(7100, BATT_WARNING);

	zassert_equal(level, BATT_WARNING, "Should stay WARNING between thresholds");
}

/* VP-18: Enter critical at 6.6V */
ZTEST(battery, test_hysteresis_enter_critical)
{
	battery_level_t level = battery_level(6500, BATT_WARNING);

	zassert_equal(level, BATT_CRITICAL);
}

/* VP-18: Enter cutoff at 6.0V */
ZTEST(battery, test_cutoff)
{
	battery_level_t level = battery_level(5900, BATT_CRITICAL);

	zassert_equal(level, BATT_CUTOFF);
}

/* EC-21: No oscillation at boundary */
ZTEST(battery, test_no_oscillation_at_warning_boundary)
{
	battery_level_t level = BATT_NORMAL;

	/* Drop to 6950 → WARNING */
	level = battery_level(6950, level);
	zassert_equal(level, BATT_WARNING);

	/* Stay at 7100 → should remain WARNING (below exit at 7200) */
	level = battery_level(7100, level);
	zassert_equal(level, BATT_WARNING);

	/* Rise to 7250 → exit to NORMAL */
	level = battery_level(7250, level);
	zassert_equal(level, BATT_NORMAL);
}

/* ===================================================================
 * MicroBlue Parser Tests (VP-14 through VP-17, EC-01 through EC-05)
 * =================================================================== */

ZTEST_SUITE(parser, NULL, NULL, NULL, NULL, NULL);

/* VP-15: Valid message parsed correctly */
ZTEST(parser, test_valid_button_message)
{
	/* [SOH] "b0" [STX] "1" [ETX] */
	uint8_t buf[] = {0x01, 'b', '0', 0x02, '1', 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_true(msg.valid);
	zassert_str_equal(msg.id, "b0");
	zassert_str_equal(msg.value, "1");
}

/* VP-15: Valid joystick message */
ZTEST(parser, test_valid_joystick_message)
{
	uint8_t buf[] = {0x01, 'j', '0', 0x02, '5', '1', '2', ',', '5', '1', '2', 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_true(msg.valid);
	zassert_str_equal(msg.id, "j0");
	zassert_str_equal(msg.value, "512,512");
}

/* VP-16: Missing SOH prefix → discarded */
ZTEST(parser, test_missing_soh)
{
	uint8_t buf[] = {'b', '0', 0x02, '1', 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_false(msg.valid, "Missing SOH must be rejected");
}

/* EC-01: Missing ETX → invalid */
ZTEST(parser, test_missing_etx)
{
	uint8_t buf[] = {0x01, 'b', '0', 0x02, '1'};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_false(msg.valid, "Missing ETX must be rejected");
}

/* EC-02: Buffer too short */
ZTEST(parser, test_too_short)
{
	uint8_t buf[] = {0x01, 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_false(msg.valid, "Too short must be rejected");
}

/* EC-04: Valid ID but empty value */
ZTEST(parser, test_empty_value)
{
	uint8_t buf[] = {0x01, 'b', '0', 0x02, 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_false(msg.valid, "Empty value must be rejected");
}

/* VP-14: Zero-length buffer */
ZTEST(parser, test_zero_length)
{
	mb_message_t msg = mb_parse(NULL, 0);

	zassert_false(msg.valid);
}

/* VP-17: Oversized ID truncated or rejected */
ZTEST(parser, test_oversized_id)
{
	/* ID longer than MB_MAX_ID_LEN */
	uint8_t buf[] = {0x01, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
			 'h',  'i', 'j', 0x02, '1', 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	/* Must either reject or truncate — but never overflow */
	if (msg.valid) {
		zassert_true(strlen(msg.id) <= MB_MAX_ID_LEN);
	}
}

/* VP-17: Oversized value truncated or rejected */
ZTEST(parser, test_oversized_value)
{
	/* Build a message with value > MB_MAX_VALUE_LEN */
	uint8_t buf[50];

	buf[0] = 0x01;
	buf[1] = 'b';
	buf[2] = '0';
	buf[3] = 0x02;
	for (int i = 4; i < 48; i++) {
		buf[i] = 'x';
	}
	buf[48] = 0x03;
	mb_message_t msg = mb_parse(buf, 49);

	if (msg.valid) {
		zassert_true(strlen(msg.value) <= MB_MAX_VALUE_LEN);
	}
}

/* VP-15: Missing STX → invalid */
ZTEST(parser, test_missing_stx)
{
	uint8_t buf[] = {0x01, 'b', '0', '1', 0x03};
	mb_message_t msg = mb_parse(buf, sizeof(buf));

	zassert_false(msg.valid, "Missing STX must be rejected");
}

/* ===================================================================
 * Calibration Tests (VP-19)
 * =================================================================== */

ZTEST_SUITE(calibration, NULL, NULL, NULL, NULL, NULL);

/* Stable samples → valid offsets */
ZTEST(calibration, test_stable_samples)
{
	int16_t samples[10][CALIB_AXES] = {
		{100, -50, 16384, 5, -3, 2},
		{102, -48, 16380, 6, -2, 1},
		{99,  -51, 16386, 4, -4, 3},
		{101, -49, 16383, 5, -3, 2},
		{100, -50, 16384, 5, -3, 2},
		{101, -50, 16385, 6, -2, 1},
		{100, -49, 16383, 5, -3, 2},
		{102, -51, 16384, 4, -4, 3},
		{99,  -50, 16385, 5, -3, 2},
		{100, -50, 16384, 5, -3, 2},
	};

	calib_result_t r = calib_compute(samples, 10);

	zassert_true(r.ok, "Stable samples should produce valid calibration");
	/* Offsets should be close to the average */
	zassert_true(r.offsets[0] >= 99 && r.offsets[0] <= 102);
}

/* VP-19: Moving drone (high variance) → rejected */
ZTEST(calibration, test_moving_drone_rejected)
{
	int16_t samples[10][CALIB_AXES] = {
		{100, -50, 16384, 5,    -3,   2},
		{500, -50, 16384, 5,    -3,   2},
		{100, 500, 16384, 5,    -3,   2},
		{100, -50, 20000, 5,    -3,   2},
		{100, -50, 16384, 1000, -3,   2},
		{100, -50, 16384, 5,    -3,   2},
		{100, -50, 16384, 5,    1000, 2},
		{100, -50, 16384, 5,    -3,   2},
		{100, -50, 16384, 5,    -3,   2},
		{100, -50, 16384, 5,    -3,   1000},
	};

	calib_result_t r = calib_compute(samples, 10);

	zassert_false(r.ok, "High variance samples must be rejected");
}

/* Single sample — edge case */
ZTEST(calibration, test_single_sample)
{
	int16_t samples[1][CALIB_AXES] = {
		{100, -50, 16384, 5, -3, 2},
	};

	calib_result_t r = calib_compute(samples, 1);

	/* With 1 sample, variance is 0 → should pass */
	zassert_true(r.ok);
	zassert_equal(r.offsets[0], 100);
}
