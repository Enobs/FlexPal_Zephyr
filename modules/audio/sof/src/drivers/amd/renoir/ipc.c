// SPDX-License-Identifier: BSD-3-Clause
//
//Copyright(c) 2021 AMD. All rights reserved.
//
//Author:       Basavaraj Hiregoudar <basavaraj.hiregoudar@amd.com>
//              Anup Kulkarni <anup.kulkarni@amd.com>
//              Bala Kishore <balakishore.pati@amd.com>

#include <rtos/panic.h>
#include <xtensa/core-macros.h>
#include <platform/chip_offset_byte.h>
#include <platform/chip_registers.h>
#include <rtos/interrupt.h>
#include <sof/ipc/driver.h>
#include <sof/ipc/msg.h>
#include <sof/ipc/schedule.h>
#include <rtos/alloc.h>
#include <sof/lib/dma.h>
#include <sof/lib/mailbox.h>
#include <sof/lib/memory.h>
#include <sof/lib/uuid.h>
#include <rtos/wait.h>
#include <sof/list.h>
#include <sof/platform.h>
#include <sof/schedule/edf_schedule.h>
#include <sof/schedule/schedule.h>
#include <rtos/task.h>
#include <rtos/spinlock.h>
#include <ipc/header.h>
#include <ipc/topology.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <platform/platform.h>

/* 49be8ff3-71a3-4456-bb7e-4723f2e5730c */
DECLARE_SOF_UUID("renoir-ipc-task", ipc_task_uuid, 0x49be8ff3, 0x71a3, 0x4456,
		 0xbb, 0x7e, 0x47, 0x23, 0xf2, 0xe5, 0x73, 0x0c);

volatile acp_scratch_mem_config_t *pscratch_mem_cfg = (volatile acp_scratch_mem_config_t *)
						      (PU_REGISTER_BASE + SCRATCH_REG_OFFSET);
static inline uint32_t sof_ipc_host_status(void)
{
	return (pscratch_mem_cfg->acp_host_ack_write | pscratch_mem_cfg->acp_host_msg_write);
}

static  inline uint32_t sof_ipc_host_msg_flag(void)
{
	return pscratch_mem_cfg->acp_host_msg_write;
}

static  inline uint32_t sof_ipc_host_ack_flag(void)
{
	return pscratch_mem_cfg->acp_host_ack_write;
}

static inline uint32_t sof_ipc_dsp_status(void)
{
	return (pscratch_mem_cfg->acp_dsp_msg_write | pscratch_mem_cfg->acp_dsp_ack_write);
}

static inline void sof_ipc_host_ack_clear(void)
{
	pscratch_mem_cfg->acp_host_ack_write = 0;
}

static inline void sof_ipc_host_msg_clear(void)
{
	pscratch_mem_cfg->acp_host_msg_write = 0;
}

static inline void sof_ipc_dsp_ack_set(void)
{
	pscratch_mem_cfg->acp_dsp_ack_write = 1;
}

static inline void sof_ipc_dsp_msg_set(void)
{
	pscratch_mem_cfg->acp_dsp_msg_write = 1;
}

static void irq_handler(void *arg)
{
	struct ipc *ipc = arg;
	uint32_t status;
	uint32_t lock;
	uint32_t delay_cnt = 10000;
	bool lock_fail = false;
	acp_dsp_sw_intr_stat_t swintrstat;
	acp_sw_intr_trig_t  swintrtrig;
	acp_future_reg_aclk_0_t acp_reg_aclk;

	swintrstat = (acp_dsp_sw_intr_stat_t)io_reg_read(PU_REGISTER_BASE + ACP_DSP_SW_INTR_STAT);
	status = swintrstat.u32all &  HOST_TO_DSP_INTR;
	acp_reg_aclk = (acp_future_reg_aclk_0_t)
				io_reg_read(PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0);
	if (status) {
		/* Interrupt source */
		if (sof_ipc_host_status()) {
			lock = io_reg_read(PU_REGISTER_BASE + ACP_AXI2DAGB_SEM_0);
			while (lock) {
				lock = io_reg_read(PU_REGISTER_BASE + ACP_AXI2DAGB_SEM_0);
				if (!delay_cnt) {
					lock_fail = true;
					break;
				}
				delay_cnt--;
			}
			if (lock_fail) {
				tr_err(&ipc_tr, "ACP fail to acquire the lock");
				return;
			}
			/* Check if it is response from host */
			if (sof_ipc_host_ack_flag()) {
				/* Clear the ACK from host  */
				sof_ipc_host_ack_clear();
				acp_reg_aclk.bits.host_aclk = 0;
				io_reg_write((PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0),
					     acp_reg_aclk.u32all);
				/* Clear the Host to DSP Status Register */
				acp_ack_intr_from_host();
				/* Configures the trigger bit in ACP_DSP_SW_INTR_TRIG register */
				swintrtrig = (acp_sw_intr_trig_t)
					     io_reg_read(PU_REGISTER_BASE + ACP_SW_INTR_TRIG);
				swintrtrig.bits.trig_host_to_dsp0_intr1	= INTERRUPT_DISABLE;
				swintrtrig.bits.trig_dsp0_to_host_intr	= INTERRUPT_DISABLE;
				io_reg_write((PU_REGISTER_BASE + ACP_SW_INTR_TRIG),
					     swintrtrig.u32all);
			}
			/* Check if new message from host */
			if (sof_ipc_host_msg_flag()) {
				/* Clear the msg bit from host */
				sof_ipc_host_msg_clear();
				acp_reg_aclk.bits.host_msg = 0;
				io_reg_write((PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0),
					     acp_reg_aclk.u32all);
				/* Clear the Host to DSP Status Register */
				acp_ack_intr_from_host();
				ipc_schedule_process(ipc);
			}
			io_reg_write((PU_REGISTER_BASE + ACP_AXI2DAGB_SEM_0), lock);
		} else {
			tr_err(&ipc_tr, "IPC:interrupt without setting flags host status 0x%x",
			       sof_ipc_host_status());
		}
	}
}

enum task_state ipc_platform_do_cmd(struct ipc *ipc)
{
	struct ipc_cmd_hdr *hdr;

	hdr = mailbox_validate();
	ipc_cmd(hdr);
	return SOF_TASK_STATE_COMPLETED;
}

void ipc_platform_complete_cmd(struct ipc *ipc)
{
	acp_sw_intr_trig_t  sw_intr_trig;
	acp_future_reg_aclk_0_t acp_reg_aclk;

	/* Set Dsp Ack for msg from host */
	sof_ipc_dsp_ack_set();
	acp_reg_aclk = (acp_future_reg_aclk_0_t)
		io_reg_read(PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0);
	acp_reg_aclk.bits.dsp_aclk = 1;
	io_reg_write((PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0), acp_reg_aclk.u32all);
	/* Configures the trigger bit in ACP_DSP_SW_INTR_TRIG register */
	sw_intr_trig = (acp_sw_intr_trig_t)
		io_reg_read(PU_REGISTER_BASE + ACP_SW_INTR_TRIG);
	sw_intr_trig.bits.trig_host_to_dsp0_intr1  = INTERRUPT_DISABLE;
	sw_intr_trig.bits.trig_dsp0_to_host_intr = INTERRUPT_DISABLE;
	io_reg_write((PU_REGISTER_BASE + ACP_SW_INTR_TRIG), sw_intr_trig.u32all);
	/* now interrupt host to tell it we have sent a message */
	acp_dsp_to_host_Intr_trig();
	if (ipc->pm_prepare_D3) {
		while (1)
			wait_for_interrupt(0);
	}
}

int ipc_platform_send_msg(const struct ipc_msg *msg)
{
	acp_sw_intr_trig_t  sw_intr_trig;
	acp_dsp_sw_intr_stat_t sw_intr_stat;
	acp_future_reg_aclk_0_t acp_reg_aclk;
	uint32_t status;
	uint32_t lock;
	uint32_t delay_cnt = 10000;
	/* Check if host cleared the status for previous messages */
	sw_intr_stat = (acp_dsp_sw_intr_stat_t)
		io_reg_read(PU_REGISTER_BASE + ACP_DSP_SW_INTR_STAT);
	status =  sw_intr_stat.bits.dsp0_to_host_intr_stat;
	if (sof_ipc_dsp_status() || status) {
		sw_intr_stat = (acp_dsp_sw_intr_stat_t)
				io_reg_read(PU_REGISTER_BASE + ACP_DSP_SW_INTR_STAT);
		status =  sw_intr_stat.bits.dsp0_to_host_intr_stat;
		return -EBUSY;
	}
	lock = io_reg_read(PU_REGISTER_BASE + ACP_AXI2DAGB_SEM_0);
	while (lock) {
		lock = io_reg_read(PU_REGISTER_BASE + ACP_AXI2DAGB_SEM_0);
		if (!delay_cnt)
			return -EBUSY;

		delay_cnt--;
	}

	/* Write new message in the mailbox */
	mailbox_dspbox_write(0, msg->tx_data, msg->tx_size);

	/* Need to set DSP message flag */
	sof_ipc_dsp_msg_set();
	acp_reg_aclk = (acp_future_reg_aclk_0_t)
		io_reg_read(PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0);
	acp_reg_aclk.bits.dsp_msg = 1;
	io_reg_write((PU_REGISTER_BASE + ACP_FUTURE_REG_ACLK_0), acp_reg_aclk.u32all);
	/* now interrupt host to tell it we have sent a message */
	acp_dsp_to_host_Intr_trig();
	/* Disable the trigger bit in ACP_DSP_SW_INTR_TRIG register */
	sw_intr_trig = (acp_sw_intr_trig_t)io_reg_read(PU_REGISTER_BASE + ACP_SW_INTR_TRIG);
	sw_intr_trig.bits.trig_dsp0_to_host_intr = INTERRUPT_DISABLE;
	io_reg_write((PU_REGISTER_BASE + ACP_SW_INTR_TRIG), sw_intr_trig.u32all);
	io_reg_write((PU_REGISTER_BASE + ACP_AXI2DAGB_SEM_0), lock);
	return 0;
}

int platform_ipc_init(struct ipc *ipc)
{
	ipc_set_drvdata(ipc, NULL);
	/* schedule */
	schedule_task_init_edf(&ipc->ipc_task, SOF_UUID(ipc_task_uuid),
			       &ipc_task_ops, ipc, 0, 0);
	arch_interrupt_clear(IRQ_NUM_EXT_LEVEL3);
	interrupt_register(IRQ_NUM_EXT_LEVEL3, irq_handler, ipc);
	/* Enabling software interuppts */
	interrupt_enable(IRQ_NUM_EXT_LEVEL3, ipc);
	return 0;
}
