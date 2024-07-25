/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <app/drivers/joystick.h>

#include <app_version.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define JOYSTICK_TEST 1

#if JOYSTICK_TEST
static const struct device *joystick_dev = DEVICE_DT_GET_ANY(uart_microblue_app);

static const struct pwm_dt_spec pwm_fl = PWM_DT_SPEC_GET(DT_NODELABEL(frontl));
static const struct pwm_dt_spec pwm_fr = PWM_DT_SPEC_GET(DT_NODELABEL(frontr));
static const struct pwm_dt_spec pwm_bl = PWM_DT_SPEC_GET(DT_NODELABEL(backl));
static const struct pwm_dt_spec pwm_br = PWM_DT_SPEC_GET(DT_NODELABEL(backr));

#define MAX_PERIOD PWM_USEC(100U)
#endif


int main(void)
{
    printk("Zephyr Minidrone Application %s\n", APP_VERSION_STRING);
#if JOYSTICK_TEST
    int ret;
    joystick_show(joystick_dev);
    if (!pwm_is_ready_dt(&pwm_fl)) {
        printk("Error: PWM device %s is not ready\n",
               pwm_fl.dev->name);
        return 0;
    }
    if (!pwm_is_ready_dt(&pwm_fr)) {
        printk("Error: PWM device %s is not ready\n",
               pwm_fr.dev->name);
        return 0;
    }
    if (!pwm_is_ready_dt(&pwm_bl)) {
        printk("Error: PWM device %s is not ready\n",
               pwm_bl.dev->name);
        return 0;
    }
    if (!pwm_is_ready_dt(&pwm_br)) {
        printk("Error: PWM device %s is not ready\n",
               pwm_br.dev->name);
        return 0;
    }
    struct joystick_data data;
    while(1){
        joystick_update(joystick_dev, K_MSEC(10));
        data = joystick_get_data(joystick_dev);
        ret = pwm_set_dt(&pwm_fl, MAX_PERIOD, MAX_PERIOD * data.left.x / 1023);
        if (ret) {
            printk("Error: PWM device %s set failed\n",
                    pwm_fl.dev->name);
            return 0;
        }
        ret = pwm_set_dt(&pwm_fr, MAX_PERIOD, MAX_PERIOD * data.left.y / 1023);
        if (ret) {
            printk("Error: PWM device %s set failed\n",
                    pwm_fr.dev->name);
            return 0;
        }
        ret = pwm_set_dt(&pwm_bl, MAX_PERIOD, MAX_PERIOD * data.right.x / 1023);
        if (ret) {
            printk("Error: PWM device %s set failed\n",
                    pwm_bl.dev->name);
            return 0;
        }
        ret = pwm_set_dt(&pwm_br, MAX_PERIOD, MAX_PERIOD * data.right.y / 1023);
        if (ret) {
            printk("Error: PWM device %s set failed\n",
                    pwm_br.dev->name);
            return 0;
        }
    }
#endif

    return 0;
}

