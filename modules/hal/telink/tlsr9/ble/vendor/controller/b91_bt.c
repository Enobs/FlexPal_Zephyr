/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include <zephyr.h>
#undef irq_enable
#undef irq_disable
#undef ARRAY_SIZE

#include "plic.h"
#include "compiler.h"
#include "b91_bt.h"
#include "b91_bt_init.h"
#include "stack/ble/controller/ble_controller.h"


/* Module defines */
#define BLE_THREAD_STACK_SIZE           CONFIG_B91_BLE_CTRL_THREAD_STACK_SIZE
#define BLE_THREAD_PRIORITY             CONFIG_B91_BLE_CTRL_THREAD_PRIORITY
#define BLE_THREAD_PERIOD_MS            CONFIG_B91_BLE_CTRL_THREAD_PERIOD_MS

#define BYTES_TO_UINT16(n, p)           {n = ((u16)(p)[0] + ((u16)(p)[1]<<8));}
#define BSTREAM_TO_UINT16(n, p)         {BYTES_TO_UINT16(n, p); p += 2;}


static struct b91_ctrl_t
{
	bool is_initialized;
	b91_bt_host_callback_t callbacks;
} b91_ctrl;

/**
 * @brief    RF driver interrupt handler
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
	blc_sdk_irq_handler();
}

/**
 * @brief    System Timer interrupt handler
 */
_attribute_ram_code_ void stimer_irq_handler(void)
{
	blc_sdk_irq_handler();
}

/**
 * @brief    BLE Controller HCI Tx callback implementation
 */
static int b91_bt_hci_tx_handler(void)
{
	/* check for data available */
	if(bltHci_txfifo.wptr == bltHci_txfifo.rptr)
	{
		return 0;
	}

	/* Get HCI data */
	u8 *p = bltHci_txfifo.p + (bltHci_txfifo.rptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
	if(p)
	{
		u32 len;
		BSTREAM_TO_UINT16(len, p);
		bltHci_txfifo.rptr++;

		/* Send data to the host */
		if(b91_ctrl.callbacks.host_read_packet)
		{
			b91_ctrl.callbacks.host_read_packet(p, len);
		}
	}

	return 0;
}

/**
 * @brief    BLE Controller HCI Rx callback implementation
 */
static int b91_bt_hci_rx_handler(void)
{
	/* Check for data available */
	if(bltHci_rxfifo.wptr == bltHci_rxfifo.rptr)
	{
		/* No data to process, send host_send_available message to the host */
		if(b91_ctrl.callbacks.host_send_available)
		{
			b91_ctrl.callbacks.host_send_available();
		}

		return 0;
	}

	/* Get HCI data */
	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.rptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	if(p)
	{
		/* Send data to the controller */
		blc_hci_handler(&p[0], 0);
		bltHci_rxfifo.rptr++;
	}

	return 0;
}

/**
 * @brief    Telink B91 BLE Controller thread
 */
static void b91_bt_controller_thread()
{
	while(1)
	{
		if(b91_ctrl.is_initialized)
		{
			blc_sdk_main_loop();
		}

		k_msleep(BLE_THREAD_PERIOD_MS);
	}
}

/**
 * @brief    BLE Controller IRQs initialization
 */
static void b91_bt_irq_init()
{
	/* Init STimer IRQ */
	IRQ_CONNECT(IRQ1_SYSTIMER, 0, stimer_irq_handler, 0, 0);

	/* Init RF IRQ */
	IRQ_CONNECT(IRQ15_ZB_RT, 0, rf_irq_handler, 0, 0);
}

/**
 * @brief    Telink B91 BLE Controller initialization
 * @return   Status - 0: command succeeded; -1: command failed
 */
int b91_bt_controller_init()
{
	int status;

	/* Init IRQs */
	b91_bt_irq_init();

	/* Init RF driver */
	rf_drv_ble_init();

	/* Init BLE Controller stack */
	status = b91_bt_blc_init(b91_bt_hci_rx_handler, b91_bt_hci_tx_handler);
	if(status != INIT_OK)
	{
		return status;
	}

	/* Init controller data */
	b91_ctrl.is_initialized = TRUE;

	return status;
}

/**
 * @brief      Host send HCI packet to controller
 * @param      data the packet point
 * @param      len the packet length
 */
void b91_bt_host_send_packet(uint8_t type, uint8_t *data, uint16_t len)
{
	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.wptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	*p++ = type;
	memcpy(p, data, len);
	bltHci_rxfifo.wptr++;
}

/**
 * @brief Register the vhci reference callback
 */
void b91_bt_host_callback_register(const b91_bt_host_callback_t *pcb)
{
	b91_ctrl.callbacks.host_read_packet = pcb->host_read_packet;
	b91_ctrl.callbacks.host_send_available = pcb->host_send_available;
}

K_THREAD_DEFINE(ZephyrBleController, BLE_THREAD_STACK_SIZE, b91_bt_controller_thread, 
                NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0, 0);
