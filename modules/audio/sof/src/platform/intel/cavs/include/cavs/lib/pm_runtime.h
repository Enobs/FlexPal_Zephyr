/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

/**
 * \file cavs/lib/pm_runtime.h
 * \brief Runtime power management header file for cAVS platforms
 * \author Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

#ifdef __PLATFORM_LIB_PM_RUNTIME_H__

#ifndef __CAVS_LIB_PM_RUNTIME_H__
#define __CAVS_LIB_PM_RUNTIME_H__

#include <stdint.h>

/**
 * \brief extra pwr flag to power up a core with a specific reason
 * it can be powered down only with the same reason (flag)
 */
#define PWRD_MASK	MASK(31, 30)
#define PWRD_BY_HPRO	BIT(31)		/**< requested by HPRO */
#define PWRD_BY_TPLG	BIT(30)		/**< typical power up */

struct pm_runtime_data;

/** \brief cAVS specific runtime power management data. */
struct cavs_pm_runtime_data {
	bool dsp_d0; /**< dsp target D0(true) or D0ix(false) */
	int host_dma_l1_sref; /**< ref counter for Host DMA accesses */
	uint32_t sleep_core_mask; /**< represents cores in waiti state */
	uint32_t prepare_d0ix_core_mask; /**< indicates whether core needs */
					   /**< to prepare to d0ix power down */
					   /**<	before next waiti */
	int dsp_client_bitmap[CONFIG_CORE_COUNT]; /**< simple pwr override */
};

/**
 * \brief Initializes platform specific runtime power management.
 * \param[in,out] prd Runtime power management data.
 */
void platform_pm_runtime_init(struct pm_runtime_data *prd);

/**
 * \brief Retrieves platform specific power management resource.
 *
 * \param[in] context Type of power management context.
 * \param[in] index Index of the device.
 * \param[in] flags Flags, set of RPM_...
 */
void platform_pm_runtime_get(uint32_t context, uint32_t index, uint32_t flags);

/**
 * \brief Releases platform specific power management resource.
 *
 * \param[in] context Type of power management context.
 * \param[in] index Index of the device.
 * \param[in] flags Flags, set of RPM_...
 */
void platform_pm_runtime_put(uint32_t context, uint32_t index, uint32_t flags);

void platform_pm_runtime_enable(uint32_t context, uint32_t index);

void platform_pm_runtime_disable(uint32_t context, uint32_t index);

void platform_pm_runtime_prepare_d0ix_en(uint32_t index);

void platform_pm_runtime_prepare_d0ix_dis(uint32_t index);

int platform_pm_runtime_prepare_d0ix_is_req(uint32_t index);

bool platform_pm_runtime_is_active(uint32_t context, uint32_t index);

/**
 * \brief Power gates platform specific hardware resources.
 */
void platform_pm_runtime_power_off(void);

/**
 * \brief CAVS DSP residency counters
 * R0 - HPRO clock, highest power consumption state
 * R1 - LPRO clock, low power consumption state
 * R2 - LPS, lowest power consumption state
 * with extra priority to R2 (LPS) which cannot be interrupted by R0/R1 changes
 */

#endif

#else

#error "Do not include outside of platform/lib/pm_runtime.h"

#endif
