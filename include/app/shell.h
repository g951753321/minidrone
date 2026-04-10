/* SPDX-License-Identifier: Apache-2.0
 *
 * Effectful shell module interfaces.
 * These modules perform I/O and bridge hardware to the pure core.
 *
 * Each module can be individually enabled via Kconfig:
 *   CONFIG_MINIDRONE_BT, CONFIG_MINIDRONE_IMU, CONFIG_MINIDRONE_MOTOR,
 *   CONFIG_MINIDRONE_BATTERY, CONFIG_MINIDRONE_LED
 * Disabled modules compile as no-op stubs.
 */

#ifndef APP_SHELL_H
#define APP_SHELL_H

#include <app/core/command.h>
#include <app/core/state_machine.h>
#include <app/core/battery.h>
#include <stdint.h>
#include <stdbool.h>

/* --- Bluetooth backend (MicroBlue over USART3) --- */

#ifdef CONFIG_MINIDRONE_BT

int shell_bt_init(void);
drone_cmd_t shell_bt_poll(void);
void shell_bt_send_battery(uint16_t voltage_mv);
void shell_bt_send_calib(bool ok);
void shell_bt_send_imu(const int16_t data[6]);
void shell_bt_send_mode(drone_mode_t mode);
void shell_bt_send_error(const char *msg);
void shell_bt_send_lowbatt(battery_level_t level);

#else

static inline int shell_bt_init(void) { return 0; }
static inline drone_cmd_t shell_bt_poll(void)
{
	return (drone_cmd_t){.type = CMD_NONE};
}
static inline void shell_bt_send_battery(uint16_t v) { (void)v; }
static inline void shell_bt_send_calib(bool ok) { (void)ok; }
static inline void shell_bt_send_imu(const int16_t d[6]) { (void)d; }
static inline void shell_bt_send_mode(drone_mode_t m) { (void)m; }
static inline void shell_bt_send_error(const char *m) { (void)m; }
static inline void shell_bt_send_lowbatt(battery_level_t l) { (void)l; }

#endif

/* --- IMU (MPU6050 via Zephyr sensor API) --- */

#ifdef CONFIG_MINIDRONE_IMU

int shell_imu_init(void);
int shell_imu_read(int16_t data[6]);

#else

static inline int shell_imu_init(void) { return 0; }
static inline int shell_imu_read(int16_t data[6])
{
	for (int i = 0; i < 6; i++) {
		data[i] = 0;
	}
	return 0;
}

#endif

/* --- Motor PWM (TIM3 CH1-4) --- */

#ifdef CONFIG_MINIDRONE_MOTOR

int shell_motor_init(void);
void shell_motor_set(uint16_t duty[4]);
void shell_motor_all_off(void);

#else

static inline int shell_motor_init(void) { return 0; }
static inline void shell_motor_set(uint16_t d[4]) { (void)d; }
static inline void shell_motor_all_off(void) {}

#endif

/* --- Battery ADC (PA5, ADC1_IN5) --- */

#ifdef CONFIG_MINIDRONE_BATTERY

int shell_battery_init(void);
uint16_t shell_battery_read_raw(void);

#else

static inline int shell_battery_init(void) { return 0; }
static inline uint16_t shell_battery_read_raw(void) { return 2500; }

#endif

/* --- LEDs (STAT_0..STAT_3) --- */

#ifdef CONFIG_MINIDRONE_LED

int shell_led_init(void);
void shell_led_set(uint8_t led_idx, bool on);
void shell_led_toggle(uint8_t led_idx);

#else

static inline int shell_led_init(void) { return 0; }
static inline void shell_led_set(uint8_t i, bool on) { (void)i; (void)on; }
static inline void shell_led_toggle(uint8_t i) { (void)i; }

#endif

#endif
