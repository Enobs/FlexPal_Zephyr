/***
 * @Author: yjwang@kunluntech.com.cn
 * @Date: 2024-04-17 13:50:21
 * @LastEditTime: 2024-04-17 13:50:21
 * @LastEditors: yjwang@kunluntech.com.cn
 * @Description:
 * @FilePath: /temperature-control/pneumatic-control-device/sensor/sensor.h
 * @
 */
#pragma once

#define SENSOR_STACK_SIZE 1024

#define SPI_FREQUENCY 18000000U // 18M
#define SPI_OPERTION (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define SENSOR_COUNT 12

void sensor_init();