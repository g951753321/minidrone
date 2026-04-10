/* SPDX-License-Identifier: Apache-2.0 */

#include <app/shell.h>
#include <app/core/microblue_parser.h>
#include <app/core/input_mapper.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(shell_bt, CONFIG_SHELL_BT_LOG_LEVEL);

static const struct device *bt_uart;

/* Ring buffer for UART RX */
#define RX_BUF_SIZE 128
static uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

/* Message assembly buffer */
static uint8_t msg_buf[MB_MAX_MSG_LEN];
static uint16_t msg_pos;
static bool msg_started;

static void uart_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uint8_t c;

			while (uart_fifo_read(dev, &c, 1) == 1) {
				uint16_t next = (rx_head + 1) % RX_BUF_SIZE;

				if (next != rx_tail) {
					rx_buf[rx_head] = c;
					rx_head = next;
				}
			}
		}
	}
}

int shell_bt_init(void)
{
	bt_uart = DEVICE_DT_GET(DT_ALIAS(bt_uart));
	if (!device_is_ready(bt_uart)) {
		return -ENODEV;
	}

	rx_head = 0;
	rx_tail = 0;
	msg_pos = 0;
	msg_started = false;

	uart_irq_callback_set(bt_uart, uart_isr);
	uart_irq_rx_enable(bt_uart);

	return 0;
}

static int rx_get(uint8_t *c)
{
	if (rx_head == rx_tail) {
		return -1;
	}
	*c = rx_buf[rx_tail];
	rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
	return 0;
}

static drone_cmd_t parse_mb_to_cmd(const mb_message_t *msg)
{
	drone_cmd_t cmd = {.type = CMD_NONE};

	if (strcmp(msg->id, "b0") == 0) {
		cmd.type = (msg->value[0] == '1') ? CMD_ARM : CMD_DISARM;
	} else if (strcmp(msg->id, "b1") == 0 && msg->value[0] == '1') {
		cmd.type = CMD_CALIBRATE;
	} else if (msg->id[0] == 'b' && msg->id[1] >= '2' && msg->id[1] <= '5') {
		if (msg->value[0] == '1') {
			cmd.type = CMD_MOTOR_TEST;
			cmd.motor_test.motor_id = msg->id[1] - '2';
		}
	} else if (msg->id[0] == 'd' && (msg->id[1] == '0' || msg->id[1] == '1')) {
		int x_raw = 512, y_raw = 512;

		sscanf(msg->value, "%d,%d", &x_raw, &y_raw);
		cmd.type = CMD_JOYSTICK;
		cmd.joystick.axis = msg->id[1] - '0';
		cmd.joystick.x = input_map_joystick((uint16_t)x_raw);
		cmd.joystick.y = input_map_joystick((uint16_t)y_raw);
	} else if (strncmp(msg->id, "sl", 2) == 0) {
		uint8_t idx = msg->id[2] - '0';

		if (idx <= 2) {
			cmd.type = CMD_SET_PID_GAIN;
			cmd.pid_gain.gain = (gain_type_t)idx;
			cmd.pid_gain.value = (uint8_t)atoi(msg->value);
		}
	}

	return cmd;
}

drone_cmd_t shell_bt_poll(void)
{
	drone_cmd_t cmd = {.type = CMD_NONE};
	uint8_t c;

	while (rx_get(&c) == 0) {
		if (c == MB_SOH) {
			msg_pos = 0;
			msg_started = true;
		}

		if (msg_started) {
			if (msg_pos < MB_MAX_MSG_LEN) {
				msg_buf[msg_pos++] = c;
			} else {
				LOG_DBG("msg overflow, dropped");
				msg_started = false;
				continue;
			}

			if (c == MB_ETX) {
				msg_started = false;
				mb_message_t msg = mb_parse(msg_buf, msg_pos);

				if (msg.valid) {
					LOG_DBG("BT rx: id=%s val=%s",
						msg.id, msg.value);
					return parse_mb_to_cmd(&msg);
				}
				LOG_DBG("parse failed (len=%u)", msg_pos);
			}
		}
	}

	return cmd;
}

static void bt_write(const char *id, const char *value)
{
	uart_poll_out(bt_uart, MB_SOH);
	while (*id) {
		uart_poll_out(bt_uart, *id++);
	}
	uart_poll_out(bt_uart, MB_STX);
	while (*value) {
		uart_poll_out(bt_uart, *value++);
	}
	uart_poll_out(bt_uart, MB_ETX);
}

void shell_bt_send_battery(uint16_t voltage_mv)
{
	char buf[8];

	snprintf(buf, sizeof(buf), "%u.%02u",
		 voltage_mv / 1000, (voltage_mv % 1000) / 10);
	bt_write("battery", buf);
}

void shell_bt_send_calib(bool ok)
{
	bt_write("calib", ok ? "ok" : "fail");
}

void shell_bt_send_imu(const int16_t data[6])
{
	char buf[48];

	snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d",
		 data[0], data[1], data[2], data[3], data[4], data[5]);
	bt_write("imu", buf);
}

void shell_bt_send_mode(drone_mode_t mode)
{
	const char *names[] = {"calibrate", "diag", "running"};

	if (mode <= DRONE_MODE_RUNNING) {
		bt_write("status", names[mode]);
	}
}

void shell_bt_send_error(const char *msg)
{
	bt_write("error", msg);
}

void shell_bt_send_lowbatt(battery_level_t level)
{
	bt_write("lowbatt", level == BATT_WARNING ? "warning" : "critical");
}
