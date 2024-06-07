/*
 * @Author: wang,yongjing
 * @Date: 2024-05-31 09:26:25
 * @LastEditTime: 2024-05-31 15:19:11
 * @LastEditors: wang,yongjing
 * @Description: motor control file
 * @FilePath: /temperature-control/pneumatic-control-device/src/lib/motor/motor.c
 * 
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include "motor.h"

/*	1- error
 *	2- warning
 *	3- info	(default)
 *	4- debug
 */
LOG_MODULE_REGISTER(motor, 4);

const struct device *pwm_dev0 = DEVICE_DT_GET(DT_NODELABEL(pwm0));
const struct device *pwm_dev2 = DEVICE_DT_GET(DT_NODELABEL(pwm2));
const struct device *pwm_dev3 = DEVICE_DT_GET(DT_NODELABEL(pwm3));
const struct device *pwm_dev7 = DEVICE_DT_GET(DT_NODELABEL(pwm7));

/**
 * @brief: set pwm api
 * @param {enum pwm_port} port
 * @param {float} percent 0.0 -100.0
 * @return {*} 0-success
 */
int set_pwm_percent(enum pwm_port port, float percent)
{
    int ret = -1;
    if (percent > 100)
        percent = 100;

    percent = percent * 0.01;

    switch (port)
    {
    case PWM1:
        ret = pwm_set_cycles(pwm_dev0, PWM_CH0, PWM0_PERIOD, percent * PWM0_PERIOD, 0);
        break;
    case PWM2:
        ret = pwm_set_cycles(pwm_dev3, PWM_CH2, PWM3_PERIOD, percent * PWM3_PERIOD, 0);
        break;
    case PWM3:
        ret = pwm_set_cycles(pwm_dev2, PWM_CH3, PWM2_PERIOD, percent * PWM2_PERIOD, 0);
        break;
    case PWM4:
        ret = pwm_set_cycles(pwm_dev3, PWM_CH3, PWM3_PERIOD, percent * PWM3_PERIOD, 0);
        break;
    case PWM5:
        ret = pwm_set_cycles(pwm_dev2, PWM_CH2, PWM2_PERIOD, percent * PWM2_PERIOD, 0);
        break;
    case PWM6:
        ret = pwm_set_cycles(pwm_dev0, PWM_CH1, PWM0_PERIOD, percent * PWM0_PERIOD, 0);
        break;
    case PWM7:
        ret = pwm_set_cycles(pwm_dev0, PWM_CH3, PWM0_PERIOD, percent * PWM0_PERIOD, 0);
        break;
    case PWM8:
        ret = pwm_set_cycles(pwm_dev0, PWM_CH2, PWM0_PERIOD, percent * PWM0_PERIOD, 0);
        break;
    case PWM9:
        ret = pwm_set_cycles(pwm_dev3, PWM_CH0, PWM3_PERIOD, percent * PWM3_PERIOD, 0);
        break;
    case PWM10:
        ret = pwm_set_cycles(pwm_dev2, PWM_CH1, PWM2_PERIOD, percent * PWM2_PERIOD, 0);
        break;
    case PWM11:
        ret = pwm_set_cycles(pwm_dev3, PWM_CH1, PWM3_PERIOD, percent * PWM3_PERIOD, 0);
        break;
    case PWM12:
        ret = pwm_set_cycles(pwm_dev2, PWM_CH0, PWM2_PERIOD, percent * PWM2_PERIOD, 0);
        break;
    default:
        LOG_ERR("pwm port select failed!");
        break;
    }
    if (ret != 0)
    {
        LOG_ERR("pwm%d set failed!", port);
        return -1;
    }
    return 0;
}

int set_fan_percent(enum fan_port port, float percent)
{
    int ret = -1;
    if (percent > 100)
        percent = 100;

    percent = (1.0 - (percent * 0.01));

    switch (port)
    {
    case FAN1:
        ret = pwm_set_cycles(pwm_dev7, FAN1, FAN_PERIOD, percent * FAN_PERIOD, 0);
        break;
    case FAN2:
        ret = pwm_set_cycles(pwm_dev7, FAN2, FAN_PERIOD, percent * FAN_PERIOD, 0);
        break;
    default:
        LOG_ERR("fan port select failed!");
        break;
    }
    if (ret != 0)
    {
        LOG_ERR("fan%d set failed!", port + 1);
        return -1;
    }
    return 0;
}
// todo servo set pwm api
int pwm_device_init()
{
    if (!device_is_ready(pwm_dev0))
    {
        LOG_ERR("pwm0 device is not ready\n");
        return -1;
    }
    if (!device_is_ready(pwm_dev2))
    {
        LOG_ERR("pwm2 device is not ready\n");
        return -1;
    }
    if (!device_is_ready(pwm_dev3))
    {
        LOG_ERR("pwm3 device is not ready\n");
        return -1;
    }
    if (!device_is_ready(pwm_dev7))
    {
        LOG_ERR("pwm7 device is not ready\n");
        return -1;
    }
    set_fan_percent(FAN1, 10);
    set_fan_percent(FAN2, 10);
    pwm_set_cycles(pwm_dev7, 2, FAN_PERIOD, 0.2 * FAN_PERIOD, 0);
    
    // init pwm output is 0
    int i;
    for(i = 0; i < PWM_MAX; i++)
    {
        set_pwm_percent(i, 0);
    }
    return 0;
}