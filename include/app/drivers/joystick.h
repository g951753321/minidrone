/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_JOYSTICK_H_
#define ZEPHYR_DRIVERS_JOYSTICK_H_

#include "zephyr/sys_clock.h"
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/toolchain.h>


struct stick_t {
    int16_t x;
    int16_t y;
};

struct joystick_data {
    struct stick_t left;
    struct stick_t right;
};

__subsystem struct joystick_driver_api {
    int (*show)(const struct device *dev);
    int (*update)(const struct device *dev, k_timeout_t timeout);
    struct joystick_data (*get_data)(const struct device *dev);
};

__syscall int joystick_show(const struct device *dev);
static inline int z_impl_joystick_show(const struct device *dev)
{
    const struct joystick_driver_api *api = dev->api;

    if (api->show == NULL) {
        return -ENOTSUP;
    }

    return api->show(dev);
}

__syscall int joystick_update(const struct device *dev, k_timeout_t timeout);
static inline int z_impl_joystick_update(const struct device *dev, k_timeout_t timeout)
{
    const struct joystick_driver_api *api = dev->api;

    if (api->update == NULL) {
        return -ENOTSUP;
    }

    return api->update(dev, timeout);
}

__syscall struct joystick_data joystick_get_data(const struct device *dev);
static inline struct joystick_data z_impl_joystick_get_data(const struct device *dev)
{
    const struct joystick_driver_api *api = dev->api;

    if (api->get_data == NULL) {
        return (struct joystick_data){0};
    }

    return api->get_data(dev);
}

#include <syscalls/joystick.h>

#endif /* ZEPHYR_DRIVERS_JOYSTICK_H_ */
