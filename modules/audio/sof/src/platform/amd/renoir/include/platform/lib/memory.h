/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2021 AMD.All rights reserved.
 *
 * Author:      Basavaraj Hiregoudar <basavaraj.hiregoudar@amd.com>
 *              Anup Kulkarni <anup.kulkarni@amd.com>
 *              Bala Kishore <balakishore.pati@amd.com>
 */
#ifdef __SOF_LIB_MEMORY_H__

#ifndef __PLATFORM_LIB_MEMORY_H__
#define __PLATFORM_LIB_MEMORY_H__

#include <rtos/cache.h>
#include <platform/chip_offset_byte.h>

/* data cache line alignment */
#define PLATFORM_DCACHE_ALIGN	64

/* physical DSP addresses */

#define IRAM_BASE	0x20000000
#define IRAM_SIZE	0x40000

#define DRAM0_BASE	0x21000000
#define DRAM0_SIZE	0x36000
#define DRAM1_SIZE	0xA8000


#define DMA0_BASE	PU_REGISTER_BASE
#define DMA0_SIZE	0x4

/* DAI DMA register base address */
#define DAI_BASE		(PU_REGISTER_BASE + ACP_I2S_RX_RINGBUFADDR)
#define DAI_SIZE		0x4
#define BT_TX_FIFO_OFFST	(ACP_BT_TX_FIFOADDR - ACP_I2S_RX_RINGBUFADDR)
#define BT_RX_FIFO_OFFST	(ACP_BT_RX_FIFOADDR - ACP_I2S_RX_RINGBUFADDR)

#define UUID_ENTRY_ELF_BASE     0x1FFFA000
#define UUID_ENTRY_ELF_SIZE     0x6000

/* Log buffer  base need to be updated properly, these are used in linker scripts */
#define LOG_ENTRY_ELF_BASE	0x20000000
#define LOG_ENTRY_ELF_SIZE	0x2000000

#define EXT_MANIFEST_ELF_BASE   (LOG_ENTRY_ELF_BASE + LOG_ENTRY_ELF_SIZE)
#define EXT_MANIFEST_ELF_SIZE   0x2000000

/* Stack configuration */
#define SOF_STACK_SIZE		(CONFIG_SOF_STACK_SIZE)
#define SOF_STACK_TOTAL_SIZE	SOF_STACK_SIZE
#define SOF_STACK_END		(DRAM0_BASE + DRAM0_SIZE)
#define SOF_STACK_BASE		(SOF_STACK_END + SOF_STACK_SIZE)

/* Mailbox configuration */
#define SRAM_OUTBOX_BASE		0x22050000
#define SRAM_OUTBOX_SIZE		0x400
#define SRAM_OUTBOX_OFFSET		0

#define SRAM_INBOX_BASE			(SRAM_OUTBOX_BASE + SRAM_OUTBOX_SIZE)
#define SRAM_INBOX_SIZE			0x400
#define SRAM_INBOX_OFFSET		SRAM_OUTBOX_SIZE

#define SRAM_DEBUG_BASE			(SRAM_INBOX_BASE + SRAM_INBOX_SIZE)
#define SRAM_DEBUG_SIZE			0x400
#define SRAM_DEBUG_OFFSET		(SRAM_INBOX_OFFSET + SRAM_INBOX_SIZE)

#define SRAM_EXCEPT_BASE		(SRAM_DEBUG_BASE + SRAM_DEBUG_SIZE)
#define SRAM_EXCEPT_SIZE		0x400
#define SRAM_EXCEPT_OFFSET		(SRAM_DEBUG_OFFSET + SRAM_DEBUG_SIZE)

#define SRAM_STREAM_BASE		(SRAM_EXCEPT_BASE + SRAM_EXCEPT_SIZE)
#define SRAM_STREAM_SIZE		0x400
#define SRAM_STREAM_OFFSET		(SRAM_EXCEPT_OFFSET + SRAM_EXCEPT_SIZE)

#define SRAM_TRACE_BASE			(SRAM_STREAM_BASE + SRAM_STREAM_SIZE)
#define SRAM_TRACE_SIZE			0x400
#define SRAM_TRACE_OFFSET		(SRAM_STREAM_OFFSET + SRAM_STREAM_SIZE)
#define SOF_MAILBOX_SIZE		(SRAM_INBOX_SIZE + SRAM_OUTBOX_SIZE \
					+ SRAM_DEBUG_SIZE + SRAM_EXCEPT_SIZE \
					+ SRAM_STREAM_SIZE + SRAM_TRACE_SIZE)

/* Heap section sizes for module pool */
#define HEAP_RT_COUNT8		0
#define HEAP_RT_COUNT16		48
#define HEAP_RT_COUNT32		48
#define HEAP_RT_COUNT64		32
#define HEAP_RT_COUNT128	48
#define HEAP_RT_COUNT256	32
#define HEAP_RT_COUNT512	4
#define HEAP_RT_COUNT1024	12
#define HEAP_RT_COUNT2048	12

/* Heap section sizes for system runtime heap */
#define HEAP_SYS_RT_COUNT64	64
#define HEAP_SYS_RT_COUNT512	8
#define HEAP_SYS_RT_COUNT1024	4

/* Heap configuration */
#define HEAP_SYSTEM_BASE	SOF_STACK_BASE
#define HEAP_SYSTEM_SIZE	0xe000
#define HEAP_SYSTEM_0_BASE	HEAP_SYSTEM_BASE
#define HEAP_SYS_RUNTIME_BASE	(HEAP_SYSTEM_BASE + HEAP_SYSTEM_SIZE)
#define HEAP_SYS_RUNTIME_SIZE	(HEAP_SYS_RT_COUNT64 * 64 + HEAP_SYS_RT_COUNT512 * 512 + \
				HEAP_SYS_RT_COUNT1024 * 1024)

#define HEAP_RUNTIME_BASE	(HEAP_SYS_RUNTIME_BASE + HEAP_SYS_RUNTIME_SIZE)
#define HEAP_RUNTIME_SIZE \
				(HEAP_RT_COUNT8 * 8 + HEAP_RT_COUNT16 * 16 + \
				HEAP_RT_COUNT32 * 32 + HEAP_RT_COUNT64 * 64 + \
				HEAP_RT_COUNT128 * 128 + HEAP_RT_COUNT256 * 256 + \
				HEAP_RT_COUNT512 * 512 + HEAP_RT_COUNT1024 * 1024 + \
				HEAP_RT_COUNT2048 * 2048)

#define HEAP_BUFFER_BASE	(HEAP_RUNTIME_BASE + HEAP_RUNTIME_SIZE)
#define HEAP_BUFFER_SIZE	(0xF000)
#define HEAP_BUFFER_BLOCK_SIZE	0x180
#define HEAP_BUFFER_COUNT	(HEAP_BUFFER_SIZE / HEAP_BUFFER_BLOCK_SIZE)

#define PLATFORM_HEAP_SYSTEM		1 /* one per core */
#define PLATFORM_HEAP_SYSTEM_RUNTIME	1 /* one per core */
#define PLATFORM_HEAP_RUNTIME		1
#define PLATFORM_HEAP_BUFFER		1

/* Vector and literal sizes - not in core-isa.h */
#define SOF_MEM_VECT_LIT_SIZE		0x7
#define SOF_MEM_VECT_TEXT_SIZE		0x37
#define SOF_MEM_VECT_SIZE		(SOF_MEM_VECT_TEXT_SIZE + SOF_MEM_VECT_LIT_SIZE)

#define SOF_MEM_RESET_TEXT_SIZE		0x400
#define SOF_MEM_RESET_LIT_SIZE		0x8
#define SOF_MEM_VECBASE_LIT_SIZE	0x178
#define SOF_MEM_WIN_TEXT_SIZE		0x178

#define SOF_MEM_RO_SIZE			0x8

#define uncache_to_cache(address)	address
#define cache_to_uncache(address)	address
#define cache_to_uncache_init(address)	address
#define is_uncached(address)		0

#define HEAP_BUF_ALIGNMENT		PLATFORM_DCACHE_ALIGN

/* brief EDF task's default stack size in bytes */
#define PLATFORM_TASK_DEFAULT_STACK_SIZE        3072

#if !defined(__ASSEMBLER__) && !defined(LINKER)
struct sof;


#define SHARED_DATA
void platform_init_memmap(struct sof *sof);

static inline void *platform_shared_get(void *ptr, int bytes)
{
	return ptr;
}

static inline void *platform_rfree_prepare(void *ptr)
{
	return ptr;
}
#endif

#endif /* __PLATFORM_LIB_MEMORY_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/memory.h"

#endif /* __SOF_LIB_MEMORY_H__ */
