/*** 
 * @Author: wang,yongjing
 * @Date: 2024-05-04 17:31:59
 * @LastEditTime: 2024-05-31 15:06:35
 * @LastEditors: wang,yongjing
 * @Description: 
 * @FilePath: /temperature-control/pneumatic-control-device/src/motor_ctrl/include/valve.h
 * @
 */
#pragma once

#define VALVE_COUNT 36

int valve_pwm_output(enum pwm_port port, float pwm_value);
int valve_init(void);