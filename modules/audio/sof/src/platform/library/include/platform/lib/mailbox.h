/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2020 Google Inc. All rights reserved.
 *
 * Author: Curtis Malainey <cujomalainey@chromium.org>
 */

#ifdef __SOF_LIB_MAILBOX_H__

#ifndef __PLATFORM_LIB_MAILBOX_H__
#define __PLATFORM_LIB_MAILBOX_H__

#include <sof/lib/memory.h>
#include <stddef.h>
#include <stdint.h>

#define MAILBOX_DSPBOX_OFFSET	0x0
#define MAILBOX_DSPBOX_SIZE	0x400
#define MAILBOX_DSPBOX_BASE \
	(MAILBOX_BASE + MAILBOX_DSPBOX_OFFSET)

#define MAILBOX_HOSTBOX_OFFSET \
	(MAILBOX_DSPBOX_SIZE + MAILBOX_DSPBOX_OFFSET)
#define MAILBOX_HOSTBOX_SIZE	0x400
#define MAILBOX_HOSTBOX_BASE \
	(MAILBOX_BASE + MAILBOX_HOSTBOX_OFFSET)

#define MAILBOX_EXCEPTION_OFFSET \
	(MAILBOX_HOSTBOX_SIZE + MAILBOX_HOSTBOX_OFFSET)
#define MAILBOX_EXCEPTION_SIZE	0x100
#define MAILBOX_EXCEPTION_BASE \
	(MAILBOX_BASE + MAILBOX_EXCEPTION_OFFSET)

#define MAILBOX_DEBUG_OFFSET \
	(MAILBOX_EXCEPTION_SIZE + MAILBOX_EXCEPTION_OFFSET)
#define MAILBOX_DEBUG_SIZE	0x100
#define MAILBOX_DEBUG_BASE \
	(MAILBOX_BASE + MAILBOX_DEBUG_OFFSET)

#define MAILBOX_STREAM_OFFSET \
	(MAILBOX_DEBUG_SIZE + MAILBOX_DEBUG_OFFSET)
/* host mailbox can be bigger to support larger and more complex use cases */
#define MAILBOX_STREAM_SIZE	0x2000
#define MAILBOX_STREAM_BASE \
	(MAILBOX_BASE + MAILBOX_STREAM_OFFSET)

#define MAILBOX_TRACE_OFFSET \
	(MAILBOX_STREAM_SIZE + MAILBOX_STREAM_OFFSET)
#define MAILBOX_TRACE_SIZE	0x380
#define MAILBOX_TRACE_BASE \
	(MAILBOX_BASE + MAILBOX_TRACE_OFFSET)

static inline void mailbox_sw_reg_write(size_t offset, uint32_t src) { }

#endif /* __PLATFORM_LIB_MAILBOX_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/mailbox.h"

#endif /* __SOF_LIB_MAILBOX_H__ */
