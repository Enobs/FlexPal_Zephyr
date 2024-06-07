// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2018 Intel Corporation. All rights reserved.
//
// Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
//         Keyon Jie <yang.jie@linux.intel.com>
//         Rander Wang <rander.wang@intel.com>
//         Janusz Jankowski <janusz.jankowski@linux.intel.com>

#include <sof/audio/component.h>
#include <rtos/timer.h>
#include <stdint.h>

void platform_host_timestamp(struct comp_dev *host,
			     struct sof_ipc_stream_posn *posn)
{
}

/* get timestamp for DAI stream DMA position */
void platform_dai_timestamp(struct comp_dev *dai,
			    struct sof_ipc_stream_posn *posn)
{
}

/* Add function to avoid tools/oss-fuzz build fail */
void platform_dai_wallclock(struct comp_dev *dai, uint64_t *wallclock)
{
	*wallclock = 0;
}

#ifndef __ZEPHYR__
uint64_t platform_timer_get(struct timer *timer)
{
	return 0;
}

uint64_t platform_timer_get_atomic(struct timer *timer)
{
	return 0;
}

void platform_timer_stop(struct timer *timer)
{
}
#endif /* __ZEPHYR__ */
