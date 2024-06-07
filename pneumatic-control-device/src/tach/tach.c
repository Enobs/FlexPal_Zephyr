#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include "tach.h"

/*	1- error
 *	2- warning
 *	3- info	(default)
 *	4- debug
 */
LOG_MODULE_REGISTER(tach, 4);

static struct k_thread tach_handle_thread;
static K_KERNEL_STACK_MEMBER(tach_handle_stack, TACH_STACK_SIZE);

#define UART_NODE2 DT_ALIAS(uart2)
const struct device *const uart2_dev = DEVICE_DT_GET(UART_NODE2);

#define TACH_MSG_SIZE 100

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart2_msgq, TACH_MSG_SIZE, 10, 4);

void serial_callback(const struct device *dev, void *user_data)
{
    uint8_t rev_data;
    /* receive buffer used in UART ISR callback */
    static char rx_buf[TACH_MSG_SIZE];
    static int rx_buf_pos;
    static bool is_receive = 0;

    if (!uart_irq_update(uart2_dev))
    {
        return;
    }

    if (!uart_irq_rx_ready(uart2_dev))
    {
        return;
    }
    /* read until FIFO empty */
    while (uart_fifo_read(uart2_dev, &rev_data, 1) == 1)
    {
        if ((rev_data == 'Z'))
        {
            is_receive = 1;
        }
        if ((rev_data == '\n') && rx_buf_pos > 0)
        {
            /* terminate string */
            rx_buf[rx_buf_pos] = '\0';

            /* if queue is full, message is silently dropped */
            k_msgq_put(&uart2_msgq, &rx_buf, K_NO_WAIT);
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
float tach[12] = {0};
static void tach_handle(void *arug0, void *arug1, void *arug2)
{
    LOG_INF("tach handle start!");

    char data_buff[TACH_MSG_SIZE];

    if (!device_is_ready(uart2_dev))
    {
        LOG_ERR("UART2 device not found!");
        return;
    }

    /* configure interrupt and callback to receive data */
    int ret = uart_irq_callback_user_data_set(uart2_dev, serial_callback, NULL);

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
    uart_irq_rx_enable(uart2_dev);
    LOG_INF("start uart input\n");
    /* indefinitely wait for input from the user */
    while (k_msgq_get(&uart2_msgq, &data_buff, K_FOREVER) == 0)
    {
        LOG_INF("data_buff: %s\n", data_buff);

        if (data_buff[0] == 'Z')
        {
            ret = sscanf(data_buff, "Z%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%fY\n",
                                    &tach[0], &tach[1], &tach[2], &tach[3],
                                    &tach[4], &tach[5], &tach[6], &tach[7],
                                    &tach[8], &tach[9], &tach[10], &tach[11]);
            if (ret != 12)
            {
                LOG_ERR("receive PWM value error, ret:%d", ret);
                continue;
            }
        }
    }
}
void tach_handle_init()
{
    k_thread_create(&tach_handle_thread, tach_handle_stack, K_THREAD_STACK_SIZEOF(tach_handle_stack),
                    tach_handle, NULL, NULL, NULL, CONFIG_MAIN_THREAD_PRIORITY, 0,
                    K_NO_WAIT);
}