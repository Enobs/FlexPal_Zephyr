/*** 
 * @Author: wang,yongjing
 * @Date: 2024-05-31 09:26:25
 * @LastEditTime: 2024-05-31 15:16:21
 * @LastEditors: wang,yongjing
 * @Description: 
 * @FilePath: /temperature-control/pneumatic-control-device/src/lib/motor/motor.h
 * @
 */
#pragma once

#define PWM0_PERIOD 33600 // 20KHz
#define PWM2_PERIOD 16800  // 20KHz
#define PWM3_PERIOD 16800  // 20KHz

#define FAN_PERIOD 8400 // 20KHz
#define SERVO_PERIOD 8400

enum pwm_port
{
    PWM1 = 0,
    PWM2,
    PWM3,
    PWM4,
    PWM5,
    PWM6,
    PWM7,
    PWM8,
    PWM9,
    PWM10,
    PWM11,
    PWM12,
    PWM_MAX
};
enum pwm_channal
{
    PWM_CH0 = 0,
    PWM_CH1,
    PWM_CH2,
    PWM_CH3,
};
enum fan_port
{
    FAN1,
    FAN2
};
int pwm_device_init(void);
int set_pwm_percent(enum pwm_port port, float percent);
int set_fan_percent(enum fan_port port, float percent);