// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2021 MediaTek. All rights reserved.
 *
 * Author: YC Hung <yc.hung@mediatek.com>
 *         Allen-KH Cheng <allen-kh.cheng@mediatek.com>
 */

#include <sof/compiler_info.h>
#include <sof/debug/debug.h>
#include <sof/drivers/edma.h>
#include <rtos/interrupt.h>
#include <sof/ipc/msg.h>
#include <rtos/timer.h>
#include <sof/fw-ready-metadata.h>
#include <sof/lib/agent.h>
#include <rtos/clk.h>
#include <sof/lib/cpu.h>
#include <sof/lib/dai.h>
#include <sof/lib/dma.h>
#include <sof/lib/mailbox.h>
#include <sof/lib/memory.h>
#include <sof/lib/mm_heap.h>
#include <sof/platform.h>
#include <sof/schedule/edf_schedule.h>
#include <sof/schedule/ll_schedule.h>
#include <sof/schedule/ll_schedule_domain.h>
#include <rtos/sof.h>
#include <sof/trace/dma-trace.h>
#include <platform/drivers/timer.h>
#include <ipc/dai.h>
#include <ipc/header.h>
#include <ipc/info.h>
#include <kernel/abi.h>
#include <kernel/ext_manifest.h>
#include <sof_versions.h>
#include <errno.h>
#include <stdint.h>
#include <xtensa/hal.h>

struct sof;

static const struct sof_ipc_fw_ready ready
	__section(".fw_ready") = {
	.hdr = {
		.cmd = SOF_IPC_FW_READY,
		.size = sizeof(struct sof_ipc_fw_ready),
	},
	/* dspbox is for DSP initiated IPC, hostbox is for host initiated IPC */
	.version = {
		.hdr.size = sizeof(struct sof_ipc_fw_version),
		.micro = SOF_MICRO,
		.minor = SOF_MINOR,
		.major = SOF_MAJOR,
		.tag = SOF_TAG,
		.abi_version = SOF_ABI_VERSION,
		.src_hash = SOF_SRC_HASH,
	},
	.flags = DEBUG_SET_FW_READY_FLAGS,
};

#define NUM_MTK_WINDOWS 6

const struct ext_man_windows xsram_window
		__aligned(EXT_MAN_ALIGN) __section(".fw_metadata") __unused = {
	.hdr = {
		.type = EXT_MAN_ELEM_WINDOW,
		.elem_size = ALIGN_UP_COMPILE(sizeof(struct ext_man_windows), EXT_MAN_ALIGN),
	},
	.window = {
		.ext_hdr	= {
			.hdr.cmd = SOF_IPC_FW_READY,
			.hdr.size = sizeof(struct sof_ipc_window),
			.type	= SOF_IPC_EXT_WINDOW,
		},
		.num_windows	= NUM_MTK_WINDOWS,
		.window	= {
			{
				.type	= SOF_IPC_REGION_UPBOX,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, /* TODO: set later */
				.size	= MAILBOX_DSPBOX_SIZE,
				.offset	= MAILBOX_DSPBOX_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_DOWNBOX,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, /* TODO: set later */
				.size	= MAILBOX_HOSTBOX_SIZE,
				.offset	= MAILBOX_HOSTBOX_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_DEBUG,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, /* TODO: set later */
				.size	= MAILBOX_DEBUG_SIZE,
				.offset	= MAILBOX_DEBUG_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_TRACE,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, /* TODO: set later */
				.size	= MAILBOX_TRACE_SIZE,
				.offset	= MAILBOX_TRACE_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_STREAM,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, /* TODO: set later */
				.size	= MAILBOX_STREAM_SIZE,
				.offset	= MAILBOX_STREAM_OFFSET,
			},
			{
				.type	= SOF_IPC_REGION_EXCEPTION,
				.id	= 0,	/* map to host window 0 */
				.flags	= 0, /* TODO: set later */
				.size	= MAILBOX_EXCEPTION_SIZE,
				.offset	= MAILBOX_EXCEPTION_OFFSET,
			},
		},
	}
};

static SHARED_DATA struct timer timer_shared = {
	.id = OSTIMER0,
	.irq = LX_ADSP_TIMTER_IRQ0_B,
};

/* Override the default MPU setup. This table matches the memory map
 * of the 'sample_controller' core and will need to be modified for
 * other cores.
 * NOTE: This table sets up all of external memory as shared uncached.
 * For best results, edit the LSP memory map to create a separate
 * section in shared memory, place all sections that need to be uncached
 * into that section, and only map that section uncached. See README
 * for more details.
 */
const unsigned int __xt_mpu_init_table_size __attribute__((section(".ResetVector.text"))) = 11;

const struct xthal_MPU_entry __xt_mpu_init_table[] __attribute__((section(".ResetVector.text"))) = {
	XTHAL_MPU_ENTRY(0x00000000, 1, XTHAL_AR_NONE,
			XTHAL_MEM_DEVICE), /* illegal: no access */
	XTHAL_MPU_ENTRY(0x0C000000, 1, XTHAL_AR_RWrw,
			XTHAL_MEM_DEVICE), /* MCU & DBG Registers read/write */
	XTHAL_MPU_ENTRY(0x0F000000, 1, XTHAL_AR_NONE,
			XTHAL_MEM_DEVICE), /* illegal: no access */
	XTHAL_MPU_ENTRY(0x10000000, 1, XTHAL_AR_RWrw,
			XTHAL_MEM_DEVICE), /* DSP register: read/write */
	XTHAL_MPU_ENTRY(0x12000000, 1, XTHAL_AR_NONE,
			XTHAL_MEM_DEVICE), /* illegal: no access */
	XTHAL_MPU_ENTRY(0x40000000, 1, XTHAL_AR_RWXrwx,
			XTHAL_MEM_WRITEBACK), /* DSP SRAM: read/write/execute writeback */
	XTHAL_MPU_ENTRY(0x40040000, 1, XTHAL_AR_NONE,
			XTHAL_MEM_DEVICE), /* illegal: no access */
	XTHAL_MPU_ENTRY(0x60000000, 1, XTHAL_AR_RWXrwx,
			XTHAL_MEM_WRITEBACK), /* DRAM: read/write/execute writeback */
	XTHAL_MPU_ENTRY(0x60800000, 1, XTHAL_AR_RWrw,
			XTHAL_MEM_WRITEBACK), /* DRAM: read/write writeback */
	XTHAL_MPU_ENTRY(0x61000000, 1, XTHAL_AR_RWrw,
			XTHAL_MEM_DEVICE), /* DMA: read/write writeback */
	XTHAL_MPU_ENTRY(0x61100000, 1, XTHAL_AR_NONE,
			XTHAL_MEM_DEVICE), /* illegal: no access */
};

int platform_boot_complete(uint32_t boot_message)
{
	mailbox_dspbox_write(0, &ready, sizeof(ready));

	/* now interrupt host to tell it we are done booting */
	trigger_irq_to_host_req();

	/* boot now complete so we can relax the CPU */
	/* For now skip this to gain more processing performance
	 * for SRC component.
	 */
	clock_set_freq(CLK_CPU(cpu_get_id()), CLK_DEFAULT_CPU_HZ);

	return 0;
}

int platform_init(struct sof *sof)
{
	int ret;

	sof->platform_timer = platform_shared_get(&timer_shared, sizeof(timer_shared));
	sof->cpu_timers = sof->platform_timer;

	platform_interrupt_init();
	platform_clock_init(sof);
	scheduler_init_edf();

	/* init low latency domains and schedulers */
	sof->platform_timer_domain = timer_domain_init(sof->platform_timer, PLATFORM_DEFAULT_CLOCK);
	scheduler_init_ll(sof->platform_timer_domain);

	platform_timer_start(sof->platform_timer);
	sa_init(sof, CONFIG_SYSTICK_PERIOD);

	clock_set_freq(CLK_CPU(cpu_get_id()), CLK_MAX_CPU_HZ);

	/* init DMA */
	ret = dmac_init(sof);
	if (ret < 0)
		return -ENODEV;

	/* Init platform domain */
	sof->platform_dma_domain = dma_multi_chan_domain_init(&sof->dma_info->dma_array[0], 1,
							      PLATFORM_DEFAULT_CLOCK, false);
	scheduler_init_ll(sof->platform_dma_domain);

	/* initialize the host IPC mechanims */
	ipc_init(sof);

	ret = dai_init(sof);
	if (ret < 0)
		return -ENODEV;

#if CONFIG_TRACE
	/* Initialize DMA for Trace*/
	trace_point(TRACE_BOOT_PLATFORM_DMA_TRACE);
	dma_trace_init_complete(sof->dmat);
#endif

	/* show heap status */
	heap_trace_all(1);

	return 0;
}

int platform_context_save(struct sof *sof)
{
	return 0;
}

void platform_wait_for_interrupt(int level)
{
	arch_wait_for_interrupt(level);
}

#ifdef __XCC__
/* This is a stub for the Xtensa libc (not their newlib version),
 * which inexplicably wants to pull in an unlink() implementation when
 * linked against the C++ standard library.  Obviously nothing in SOF
 * uses the C library filesystem layer, this is just spurious.
 */
int __attribute__((weak)) _unlink_r(struct _reent *ptr, const char *file);
int __attribute__((weak)) _unlink_r(struct _reent *ptr, const char *file)
{
	return -1;
}
#endif
