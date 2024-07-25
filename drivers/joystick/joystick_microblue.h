/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_JOYSTICK_JOYSTICK_MICROBLUE_H_
#define ZEPHYR_DRIVERS_JOYSTICK_JOYSTICK_MICROBLUE_H_

#include <zephyr/device.h>
#include <app/drivers/joystick.h>

#define DEV_CFG(dev) \
    ((struct joystick_microblue_config *)(dev)->config)

#define DEV_DATA(dev) \
    ((struct joystick_data *)(dev)->data)

#define MAX_ID_LEN 8
#define MAX_VALUE_LEN 2
#define JOYSTICK_THREAD_STACK_SIZE 512

typedef const struct device *joystick_mb_t;
typedef const struct joystick_microblue_config *joystick_mb_cfg_t;
typedef const struct joystick_data *joystick_mb_data_t;

struct joystick_microblue_config {
    const struct device *uart_dev;
    char left_id[MAX_ID_LEN];
    char right_id[MAX_ID_LEN];
};

enum MB_CMD_TOKEN {
    MICROBLUE_CMD_START = 0x01,
    MICROBLUE_CMD_TEXT = 0x02,
    MICROBLUE_CMD_TEXTSEP = 0x2c, // The seperate is ',' in ASCII
    MICROBLUE_CMD_END = 0x03,
};

struct microblue_data_t {
    intptr_t _unused;
    char ID[MAX_ID_LEN];
    int16_t value[MAX_VALUE_LEN];
    uint8_t value_len;
};


#endif /* ZEPHYR_DRIVERS_JOYSTICK_JOYSTICK_MICROBLUE_H_ */
