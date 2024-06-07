// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Google Inc. All rights reserved.
//
// Author: Curtis Malainey <cujomalainey@chromium.org>

#include <rtos/sof.h>
#include <sof/ipc/driver.h>
#include <rtos/timer.h>
#include <sof/lib/agent.h>
#include <sof/schedule/edf_schedule.h>
#include <sof/schedule/ll_schedule_domain.h>
#include <sof/lib/mailbox.h>
#include <sof/lib/dai.h>

#ifndef __ZEPHYR__
static SHARED_DATA struct timer timer = {};
#endif /* __ZEPHYR__ */

static uint8_t mailbox[MAILBOX_DSPBOX_SIZE +
		       MAILBOX_HOSTBOX_SIZE +
		       MAILBOX_EXCEPTION_SIZE +
		       MAILBOX_DEBUG_SIZE +
		       MAILBOX_STREAM_SIZE +
		       MAILBOX_TRACE_SIZE];

uint8_t *get_library_mailbox()
{
	return mailbox;
}

static void platform_clock_init(struct sof *sof) {}

int dmac_init(struct sof *sof)
{
	return 0;
}

int platform_init(struct sof *sof)
{
#ifndef __ZEPHYR__
	sof->platform_timer = &timer;
	sof->cpu_timers = &timer;
#endif

	platform_clock_init(sof);

	scheduler_init_edf();

	/* init low latency timer domain and scheduler */
	/* sof->platform_timer_domain = */
	/* timer_domain_init(sof->platform_timer, PLATFORM_DEFAULT_CLOCK, */
	/* CONFIG_SYSTICK_PERIOD); */

	/* init the system agent */
	sa_init(sof, CONFIG_SYSTICK_PERIOD);

	/* init DMACs */
	dmac_init(sof);

	/* initialise the host IPC mechanisms */
	ipc_init(sof);

	dai_init(sof);

	return 0;
}

int platform_context_save(struct sof *sof)
{
	return 0;
}

#ifdef __ZEPHYR__
/* Stubs for unsupported architectures */

/* Platform */
int platform_boot_complete(uint32_t boot_message)
{
	return 0;
}

/* Logging */
LOG_MODULE_REGISTER(sof);

#endif
