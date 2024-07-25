/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel/thread_stack.h"
#define DT_DRV_COMPAT uart_microblue_app

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <app/drivers/joystick.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(joystick, CONFIG_JOYSTICK_LOG_LEVEL);

#include "joystick_microblue.h"

static K_FIFO_DEFINE(cmd_queue);
static K_FIFO_DEFINE(process_queue);
static struct microblue_data_t cmd_buf[2];

static int read_uart(const struct device *uart, uint8_t *buf,
             unsigned int size)
{
    int rx;

    rx = uart_fifo_read(uart, buf, size);
    if (rx < 0)
    {
        /* Overrun issue. Stop the UART */
        uart_irq_rx_disable(uart);
        return -EIO;
    }

    return rx;
}

static void uart_dev_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);
    static uint8_t id_idx = 0, value_idx = 0, drop_until_end = 0;
    static enum MB_CMD_TOKEN state = MICROBLUE_CMD_START;
    static struct microblue_data_t *cmd;
    while (uart_irq_update(dev) > 0 &&
           uart_irq_is_pending(dev) > 0)
    {
        uint8_t byte;
        bool is_token = false;
        int rx;

        rx = uart_irq_rx_ready(dev);
        if (rx < 0)
            return;

        if (rx == 0)
            continue;

        read_uart(dev, &byte, 1);

        if (!cmd)
        {
            cmd = k_fifo_get(&process_queue, K_NO_WAIT);
            if (!cmd)
            {
                LOG_ERR("No cmd available\n");
                return;
            }
        }
        // Store the state of the command
        switch(byte)
        {
            case MICROBLUE_CMD_START:
                state = MICROBLUE_CMD_START;
                memset(cmd, 0, sizeof(struct microblue_data_t));
                id_idx = 0;
                is_token = true;
                break;
            case MICROBLUE_CMD_TEXT:
                state = MICROBLUE_CMD_TEXT;
                value_idx = 0;
                is_token = true;
                break;
            case MICROBLUE_CMD_TEXTSEP:
                state = MICROBLUE_CMD_TEXT;
                value_idx++;
                if (value_idx >= MAX_VALUE_LEN)
                {
                    drop_until_end = 1;
                }
                is_token = true;
                break;
            case MICROBLUE_CMD_END:
                state = MICROBLUE_CMD_END;
                cmd->value_len = value_idx+1;
                k_fifo_put(&cmd_queue, cmd);
                cmd = NULL;
                is_token = false;
                drop_until_end = 0;
                break;
            default:
                is_token = false;
                break;
        }

        if (!is_token && !drop_until_end)
        {
            switch(state)
            {
                case MICROBLUE_CMD_START:
                    cmd->ID[id_idx++] = byte;
                    break;
                case MICROBLUE_CMD_TEXT:
                    cmd->value[value_idx] = cmd->value[value_idx] * 10 + (byte - '0');
                    break;
                default:
                    break;
            }
        }
    }
}

static void uart_input_init(const struct device *dev)
{
    uint8_t c;

    uart_irq_rx_disable(dev);
    uart_irq_tx_disable(dev);

    uart_irq_callback_set(dev, uart_dev_isr);

    for (int i = 0; i < ARRAY_SIZE(cmd_buf); i++) {
        k_fifo_put(&process_queue, &cmd_buf[i]);
    }

    /* Drain the fifo */
    while (uart_irq_rx_ready(dev) > 0) {
        uart_fifo_read(dev, &c, 1);
    }

    uart_irq_rx_enable(dev);
}

int joystick_microblue_show(const struct device *dev)
{
    joystick_mb_cfg_t config = DEV_CFG(dev);
    joystick_mb_data_t data = DEV_DATA(dev);
    LOG_INF("Joystick (%s) information:", dev->name);
    LOG_INF("Left stick ID: %s", config->left_id);
    LOG_INF("Right stick ID: %s", config->right_id);
    LOG_INF("Left stick x, y: %d, %d", data->left.x, data->left.y);
    LOG_INF("Right stick x, y: %d, %d", data->right.x, data->right.y);
    return 0;
}

int joystick_microblue_update(const struct device *dev, k_timeout_t timeout)
{
    joystick_mb_cfg_t config = DEV_CFG(dev);
    struct joystick_data *data = DEV_DATA(dev);
    struct microblue_data_t *cmd;
    cmd = k_fifo_get(&cmd_queue, timeout);
    if (!cmd)
    {
        return -ENODATA;
    }
    if (strcmp(cmd->ID, config->left_id) == 0)
    {
        data->left.x = cmd->value[0];
        data->left.y = cmd->value[1];
    }
    else if (strcmp(cmd->ID, config->right_id) == 0)
    {
        data->right.x = cmd->value[0];
        data->right.y = cmd->value[1];
    }
    k_fifo_put(&process_queue, cmd);
    return 0;
}

struct joystick_data joystick_microblue_get_data(const struct device *dev)
{
    joystick_mb_data_t data = DEV_DATA(dev);
    return (struct joystick_data){
        .left = data->left,
        .right = data->right,
    };
}

static int joystick_microblue_init(joystick_mb_t dev)
{
    joystick_mb_cfg_t config = DEV_CFG(dev);
    LOG_DBG("Joystick Microblue driver init");
    uart_input_init(config->uart_dev);
    return 0;
}

static const struct joystick_driver_api joystick_microblue_driver_api = {
    .show = joystick_microblue_show,
    .update = joystick_microblue_update,
    .get_data = joystick_microblue_get_data,
};

#define JOYSTICK_MICROBLUE_INIT(n) \
    static const struct joystick_microblue_config joystick_microblue_config_##n = { \
        .uart_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, uart)), \
        .left_id = DT_INST_PROP(n, left_stick_id), \
        .right_id = DT_INST_PROP(n, right_stick_id), \
    }; \
    static struct joystick_data joystick_microblue_data_##n = { \
        .left = {512, 512}, \
        .right = {512, 512}, \
    }; \
    DEVICE_DT_INST_DEFINE(n, joystick_microblue_init, NULL, \
                          &joystick_microblue_data_##n, &joystick_microblue_config_##n, \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, \
                          &joystick_microblue_driver_api);

DT_INST_FOREACH_STATUS_OKAY(JOYSTICK_MICROBLUE_INIT)
