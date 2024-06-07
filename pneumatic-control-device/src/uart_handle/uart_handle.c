/*
 * @Author: yjwang@kunluntech.com.cn
 * @Date: 2024-05-02 16:43:56
 * @LastEditTime: 2024-06-03 17:42:45
 * @LastEditors: wang,yongjing
 * @Description:
 * @FilePath: /temperature-control/pneumatic-control-device/src/uart_handle/uart_handle.c
 *
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include "uart_handle.h"
#include "motor.h"
#include "valve.h"

/*	1- error
 *	2- warning
 *	3- info	(default)
 *	4- debug
 */
LOG_MODULE_REGISTER(uart, 4);

static struct k_thread uart_handle_thread;
static K_KERNEL_STACK_MEMBER(uart_handle_stack, UART_STACK_SIZE);

#define UART_NODE1 DT_ALIAS(uart1)
const struct device *const uart1_dev = DEVICE_DT_GET(UART_NODE1);

#define MSG_SIZE 100

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

void serial_cb(const struct device *dev, void *user_data)
{
    uint8_t rev_data;
    /* receive buffer used in UART ISR callback */
    static char rx_buf[MSG_SIZE];
    static int rx_buf_pos;
    static bool is_receive = 0;

    if (!uart_irq_update(uart1_dev))
    {
        return;
    }

    if (!uart_irq_rx_ready(uart1_dev))
    {
        return;
    }
    /* read until FIFO empty */
    while (uart_fifo_read(uart1_dev, &rev_data, 1) == 1)
    {
        if ((rev_data == 'Z') || (rev_data == 'A'))
        {
            is_receive = 1;
        }
        if ((rev_data == '\n') && rx_buf_pos > 0)
        {
            /* terminate string */
            rx_buf[rx_buf_pos] = '\0';

            /* if queue is full, message is silently dropped */
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
            /* reset the buffer (it was copied to the msgq) */
            rx_buf_pos = 0;
            is_receive = 0;
        }
        else if (rx_buf_pos < (sizeof(rx_buf) - 1) && is_receive)
        {
            rx_buf[rx_buf_pos++] = rev_data;
        }
        /* else: characters beyond buffer size are dropped */
    }
}
float receive_value[12] = {0};
static void uart_handle(void *arug0, void *arug1, void *arug2)
{
    LOG_INF("uart handle start!");
    valve_init();

    char data_buff[MSG_SIZE];

    if (!device_is_ready(uart1_dev))
    {
        LOG_ERR("UART device not found!");
        return;
    }

    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart1_dev, serial_cb, NULL);

    if (ret < 0)
    {
        if (ret == -ENOTSUP)
        {
            LOG_ERR("Interrupt-driven UART API support not enabled\n");
        }
        else if (ret == -ENOSYS)
        {
            LOG_ERR("UART device does not support interrupt-driven API\n");
        }
        else
        {
            LOG_ERR("Error setting UART callback: %d\n", ret);
        }
        return;
    }
    uart_irq_rx_enable(uart1_dev);
    LOG_INF("start uart input\n");
    /* indefinitely wait for input from the user */
    while (k_msgq_get(&uart_msgq, &data_buff, K_FOREVER) == 0)
    {
        LOG_INF("data_buff: %s\n", data_buff);

        // receive air pneumatic value
        if (data_buff[0] == 'A')
        {
            ret = sscanf(data_buff, "A%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%fY\n",
                                    &receive_value[0], &receive_value[1], &receive_value[2], &receive_value[3],
                                    &receive_value[4], &receive_value[5], &receive_value[6], &receive_value[7],
                                    &receive_value[8], &receive_value[9], &receive_value[10], &receive_value[11]);
            if (ret != PWM_MAX)
            {
                LOG_ERR("receive air value error, ret:%d", ret);
                continue;
            }
            // todo pid control
            LOG_INF("receive air value");
        }
        else if (data_buff[0] == 'Z')
        {
            ret = sscanf(data_buff, "Z%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%fY\n",
                                    &receive_value[0], &receive_value[1], &receive_value[2], &receive_value[3],
                                    &receive_value[4], &receive_value[5], &receive_value[6], &receive_value[7],
                                    &receive_value[8], &receive_value[9], &receive_value[10], &receive_value[11]);
            if (ret != PWM_MAX)
            {
                LOG_ERR("receive PWM value error, ret:%d", ret);
                continue;
            }
        }

        for (int i = 0; i < PWM_MAX; i++)
        {
            valve_pwm_output(i, receive_value[i]);
        }
    }
}
void uart_handle_init()
{
    k_thread_create(&uart_handle_thread, uart_handle_stack, K_THREAD_STACK_SIZEOF(uart_handle_stack),
                    uart_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
                    K_NO_WAIT);
}