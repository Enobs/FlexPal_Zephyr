/*
 * @Author: wang,yongjing
 * @Date: 2024-05-31 09:26:25
 * @LastEditTime: 2024-05-31 15:19:58
 * @LastEditors: wang,yongjing
 * @Description: valve control file, must depend on motor.c
 * @FilePath: /temperature-control/pneumatic-control-device/src/lib/valve/valve.c
 * 
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "motor.h"
#include "valve.h"

/*	1- error
 *	2- warning
 *	3- info	(default)
 *	4- debug
 */
LOG_MODULE_REGISTER(valve, 4);

static const struct gpio_dt_spec valve_gpio[VALVE_COUNT] = {
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 0),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 1),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 2),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 3),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 4),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 5),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 6),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 7),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 8),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 9),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 10),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 11),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 12),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 13),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 14),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 15),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 16),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 17),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 18),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 19),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 20),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 21),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 22),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 23),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 24),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 25),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 26),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 27),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 28),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 29),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 30),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 31),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 32),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 33),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 34),
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(air_valve), valve_gpios, 35)};

static bool valve_map[2][3] = {
    {1, 0, 0}, // inlet
    {0, 1, 1}  // outlet
};
static inline float custom_abs(float value)
{
    return (value > 0 ? value : -value);
}
/**
 * @brief: set valve active
 * @param {enum pwm_port} port
 * @param {bool} mode: 0-inlet 1-outlet
 * @return {*}
 */
int set_valve_active(enum pwm_port port, bool mode)
{
    int i;

    switch (port)
    {
    case PWM1:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i], valve_map[mode][i]);
        } break;
    case PWM2:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+3], valve_map[mode][i]);
        } break;
    case PWM3:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+6], valve_map[mode][i]);
        } break;
    case PWM4:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+9], valve_map[mode][i]);
        }
    case PWM5:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+12], valve_map[mode][i]);
        } break;
    case PWM6:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+15], valve_map[mode][i]);
        }
    case PWM7:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+18], valve_map[mode][i]);
        } break;
    case PWM8:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+21], valve_map[mode][i]);
        }
    case PWM9:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+24], valve_map[mode][i]);
        } break;
    case PWM10:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+27], valve_map[mode][i]);
        }break;
    case PWM11:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+30], valve_map[mode][i]);
        } break;
    case PWM12:
        for (i = 0; i < 3; i++) {
            gpio_pin_set_dt(&valve_gpio[i+33], valve_map[mode][i]);
        }
    default:
        break;
    }
    return 0;
}
int valve_pwm_output(enum pwm_port port, float pwm_value)
{
    bool mode = 0;

    if (pwm_value > 0)
        mode = 1;

    set_valve_active(port, mode);
    set_pwm_percent(port, custom_abs(pwm_value));

    return 0;
}

int valve_init(void)
{
    int i;
    pwm_device_init();
    
    for (i = 0; i < VALVE_COUNT; i++)
    {
        if (!gpio_is_ready_dt(&valve_gpio[i]))
        {
            LOG_ERR("GPIO controller value%d not ready", i);
            return -1;
        }
    }

    for (i = 0; i < VALVE_COUNT; i++)
    {
        if (gpio_pin_configure_dt(&valve_gpio[i], GPIO_OUTPUT) != 0)
        {
            LOG_ERR("GPIO controller value%d config failed", i);
            return -1;
        }
    }

    return 0;
}