#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include "sensor.h"
#include "uart_handle.h"
#include "tach.h"

/*	1- error
 *	2- warning
 *	3- info	(default)
 *	4- debug
 */
LOG_MODULE_REGISTER(main, 4);

int main(void)
{
    LOG_INF("Welcome to The Pneumatic Control Device!");
    sensor_init();
    uart_handle_init();
    tach_handle_init();

    return 0;
}
