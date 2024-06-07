// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2018 Intel Corporation. All rights reserved.
//
// Author: Tomasz Lauda <tomasz.lauda@linux.intel.com>
//         Janusz Jankowski <janusz.jankowski@linux.intel.com>

/**
 * \file
 * \brief Runtime power management implementation for Tiger Lake
 * \author Tomasz Lauda <tomasz.lauda@linux.intel.com>
 */

#include <cavs/lib/pm_memory.h>
#include <cavs/version.h>
#include <rtos/panic.h>
#include <rtos/interrupt.h>
#include <rtos/alloc.h>
#include <sof/lib/dai.h>
#include <sof/lib/io.h>
#include <sof/lib/memory.h>
#include <sof/lib/pm_runtime.h>
#include <sof/lib/shim.h>
#include <sof/lib/uuid.h>
#include <rtos/wait.h>
#include <sof/platform.h>
#include <rtos/spinlock.h>
#include <sof/trace/trace.h>
#include <ipc/topology.h>
#include <user/trace.h>
#include <cavs/drivers/sideband-ipc.h>

#include <sof_versions.h>
#include <stdint.h>

#include <cavs/lib/power_down.h>

LOG_MODULE_REGISTER(power, CONFIG_SOF_LOG_LEVEL);

/* 76cc9773-440c-4df9-95a8-72defe7796fc */
DECLARE_SOF_UUID("power", power_uuid, 0x76cc9773, 0x440c, 0x4df9,
		 0x95, 0xa8, 0x72, 0xde, 0xfe, 0x77, 0x96, 0xfc);

DECLARE_TR_CTX(power_tr, SOF_UUID(power_uuid), LOG_LEVEL_INFO);

/*
 * To support Zephyr, some adaptation is needed to the driver.
 */
#ifdef __ZEPHYR__
extern int cpu_enable_secondary_core(int id);
#endif

/**
 * \brief Registers Host DMA usage that should not trigger
 * transition to L0 via forced L1 exit.
 */
static void cavs_pm_runtime_host_dma_l1_get(void)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&prd->lock);

	pprd->host_dma_l1_sref++;

	k_spin_unlock(&prd->lock, key);
}

/**
 * \brief Releases Host DMA usage preventing L1 exit. If this
 * the last user, forced L1 exit is performed.
 */
static inline void cavs_pm_runtime_host_dma_l1_put(void)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&prd->lock);

	if (!--pprd->host_dma_l1_sref) {
		shim_write(SHIM_SVCFG,
			   shim_read(SHIM_SVCFG) | SHIM_SVCFG_FORCE_L1_EXIT);

		wait_delay(PLATFORM_FORCE_L1_EXIT_TIME);

		shim_write(SHIM_SVCFG,
			   shim_read(SHIM_SVCFG) & ~(SHIM_SVCFG_FORCE_L1_EXIT));
	}

	k_spin_unlock(&prd->lock, key);
}

static inline void cavs_pm_runtime_enable_dsp(bool enable)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	uint32_t flags;

	/* request is always run on dsp0 and applies to dsp0,
	 * so no global lock is required.
	 */
	irq_local_disable(flags);

	pprd->dsp_d0 = !enable;

	irq_local_enable(flags);

	tr_info(&power_tr, "pm_runtime_enable_dsp dsp_d0_sref %d",
		pprd->dsp_d0);

#if CONFIG_DSP_RESIDENCY_COUNTERS
	struct clock_info *clk_info = clocks_get() + CLK_CPU(cpu_get_id());

	if (!clk_info)
		return;

	if (pprd->dsp_d0) {
		if (clk_info->current_freq_idx == CPU_LPRO_FREQ_IDX)
			report_dsp_r_state(r1_r_state);
		else
			report_dsp_r_state(r0_r_state);
	} else {
		report_dsp_r_state(r2_r_state);
	}
#endif
}

static inline bool cavs_pm_runtime_is_active_dsp(void)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;

	/* even if dsp_d0 is false (dsp in D0ix state) function will return true
	 * until secondary cores be prepared of d0ix power down.
	 */
	return pprd->dsp_d0 || pprd->prepare_d0ix_core_mask;
}

#if CONFIG_INTEL_SSP
static inline void cavs_pm_runtime_en_ssp_power(uint32_t index)
{
#if CONFIG_TIGERLAKE
	uint32_t reg;

	tr_info(&power_tr, "en_ssp_power index %d", index);

	io_reg_write(I2SLCTL, io_reg_read(I2SLCTL) | I2SLCTL_SPA(index));

	/* Check if powered on. */
	do {
		reg = io_reg_read(I2SLCTL);
	} while (!(reg & I2SLCTL_CPA(index)));

	tr_info(&power_tr, "en_ssp_power I2SLCTL %08x", reg);
#endif
}

static inline void cavs_pm_runtime_dis_ssp_power(uint32_t index)
{
#if CONFIG_TIGERLAKE
	uint32_t reg;

	tr_info(&power_tr, "dis_ssp_power index %d", index);

	io_reg_write(I2SLCTL, io_reg_read(I2SLCTL) & (~I2SLCTL_SPA(index)));

	/* Check if powered off. */
	do {
		reg = io_reg_read(I2SLCTL);
	} while (reg & I2SLCTL_CPA(index));

	tr_info(&power_tr, "dis_ssp_power I2SLCTL %08x", reg);
#endif
}
#endif

#if CONFIG_INTEL_DMIC
static inline void cavs_pm_runtime_dis_dmic_clk_gating(uint32_t index)
{
#if CONFIG_TIGERLAKE
	/* Disable DMIC clock gating */
	io_reg_write(DMICLCTL,
		    (io_reg_read(DMICLCTL) | DMIC_DCGD));
#endif
}

static inline void cavs_pm_runtime_en_dmic_clk_gating(uint32_t index)
{
#if CONFIG_TIGERLAKE
	/* Enable DMIC clock gating */
	io_reg_write(DMICLCTL,
		    (io_reg_read(DMICLCTL) & ~DMIC_DCGD));
#endif
}
static inline void cavs_pm_runtime_en_dmic_power(uint32_t index)
{
	(void) index;
#if CONFIG_TIGERLAKE
	/* Enable DMIC power */
	io_reg_write(DMICLCTL,
		    (io_reg_read(DMICLCTL) | DMICLCTL_SPA));
#endif
}
static inline void cavs_pm_runtime_dis_dmic_power(uint32_t index)
{
	(void) index;
#if CONFIG_TIGERLAKE
	/* Disable DMIC power */
	io_reg_write(DMICLCTL,
		    (io_reg_read(DMICLCTL) & (~DMICLCTL_SPA)));
#endif
}
#endif /* #if defined(CONFIG_INTEL_DMIC) */

#ifdef __ZEPHYR__
/* TODO: Zephyr has it's own core start */
static inline void cavs_pm_runtime_core_dis_memory(uint32_t index)
{
}

static inline void cavs_pm_runtime_core_en_memory(uint32_t index)
{
}

#else

static inline void cavs_pm_runtime_core_dis_memory(uint32_t index)
{
	void *core_memory_ptr;
	extern uintptr_t _sof_core_s_start;

	/* Address is calculated for index (0 for the primary core) minus one
	 * since _sof_core_s_start is first secondary core stack address
	 */
	core_memory_ptr = (char *)&_sof_core_s_start
		+ (index - 1) * SOF_CORE_S_SIZE;

	cavs_pm_memory_hp_sram_power_gate(core_memory_ptr, SOF_CORE_S_SIZE,
					  false);
}

static inline void cavs_pm_runtime_core_en_memory(uint32_t index)
{
	void *core_memory_ptr;
	extern uintptr_t _sof_core_s_start;

	/* Address is calculated for index (0 for the primary core) minus one
	 * since _sof_core_s_start is first secondary core stack address
	 */
	core_memory_ptr = (char *)&_sof_core_s_start
		+ (index - 1) * SOF_CORE_S_SIZE;

	cavs_pm_memory_hp_sram_power_gate(core_memory_ptr, SOF_CORE_S_SIZE,
					  true);
}
#endif

static inline void cavs_pm_runtime_core_dis_hp_clk(uint32_t index)
{
	int all_active_cores_sleep;
	int enabled_cores = cpu_enabled_cores();
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&prd->lock);

	pprd->sleep_core_mask |= BIT(index);

	all_active_cores_sleep =
		(enabled_cores & pprd->sleep_core_mask) == enabled_cores;

	if (all_active_cores_sleep)
		clock_low_power_mode(CLK_CPU(index), true);

	k_spin_unlock(&prd->lock, key);
}

static inline void cavs_pm_runtime_core_en_hp_clk(uint32_t index)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&prd->lock);

	pprd->sleep_core_mask &= ~BIT(index);
	clock_low_power_mode(CLK_CPU(index), false);

	k_spin_unlock(&prd->lock, key);
}

static inline void cavs_pm_runtime_dis_dsp_pg(uint32_t index)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	uint32_t lps_ctl, tries = PLATFORM_PM_RUNTIME_DSP_TRIES;
	uint32_t flag = PWRD_MASK & index;

	index &= ~PWRD_MASK;

	if (index == PLATFORM_PRIMARY_CORE_ID) {
		lps_ctl = shim_read(SHIM_LPSCTL);

		shim_write16(SHIM_PWRCTL, shim_read16(SHIM_PWRCTL) |
			     SHIM_PWRCTL_TCPDSPPG(index) |
			     SHIM_PWRCTL_TCPCTLPG);

		lps_ctl &= ~SHIM_LPSCTL_BID;
		lps_ctl &= ~SHIM_LPSCTL_BATTR_0;
		lps_ctl |= SHIM_LPSCTL_FDSPRUN;
		shim_write(SHIM_LPSCTL, lps_ctl);
	} else {
#ifdef __ZEPHYR__
		/*
		 * In Zephyr secondary power-up needs to go via Zephyr
		 * SMP kernel core, so we can't program PWRCTL directly here.
		 */
		cpu_enable_secondary_core(index);
#else
		/* Secondary core power up */
		shim_write16(SHIM_PWRCTL, shim_read16(SHIM_PWRCTL) |
			     SHIM_PWRCTL_TCPDSPPG(index) |
			     SHIM_PWRCTL_TCPCTLPG);
#endif

		/* Waiting for power up */
		while (((shim_read16(SHIM_PWRSTS) & SHIM_PWRCTL_TCPDSPPG(index)) !=
			SHIM_PWRCTL_TCPDSPPG(index)) && tries--) {
			idelay(PLATFORM_PM_RUNTIME_DSP_DELAY);
		}
		/* Timeout check with warning log */
		if (tries == 0)
			tr_err(&power_tr, "cavs_pm_runtime_dis_dsp_pg(): failed to power up core %d",
			       index);
		pprd->dsp_client_bitmap[index] |= flag;
	}
}

static inline void cavs_pm_runtime_en_dsp_pg(uint32_t index)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	uint32_t lps_ctl;
	uint32_t flag = PWRD_MASK & index;

	index &= ~(PWRD_MASK);

	if (index == PLATFORM_PRIMARY_CORE_ID) {
		lps_ctl = shim_read(SHIM_LPSCTL);

		shim_write16(SHIM_PWRCTL, 0);

		lps_ctl |= SHIM_LPSCTL_BID | SHIM_LPSCTL_BATTR_0;
		lps_ctl &= ~SHIM_LPSCTL_FDSPRUN;
		shim_write(SHIM_LPSCTL, lps_ctl);
	} else {
		pprd->dsp_client_bitmap[index] &= ~(flag);

		if (pprd->dsp_client_bitmap[index] == 0)
			shim_write16(SHIM_PWRCTL, shim_read16(SHIM_PWRCTL) &
				     ~SHIM_PWRCTL_TCPDSPPG(index));
	}
}

void platform_pm_runtime_init(struct pm_runtime_data *prd)
{
	struct cavs_pm_runtime_data *pprd;

	pprd = rzalloc(SOF_MEM_ZONE_SYS_SHARED, 0, SOF_MEM_CAPS_RAM, sizeof(*pprd));
	prd->platform_data = pprd;
}

void platform_pm_runtime_get(enum pm_runtime_context context, uint32_t index,
			     uint32_t flags)
{
	/* Action based on context */
	switch (context) {
	case PM_RUNTIME_HOST_DMA_L1:
		cavs_pm_runtime_host_dma_l1_get();
		break;
#if CONFIG_INTEL_SSP
	case SSP_CLK:
		break;
	case SSP_POW:
		cavs_pm_runtime_en_ssp_power(index);
		break;
#endif
#if CONFIG_INTEL_DMIC
	case DMIC_CLK:
		cavs_pm_runtime_dis_dmic_clk_gating(index);
		break;
	case DMIC_POW:
		cavs_pm_runtime_en_dmic_power(index);
		break;
#endif
	case DW_DMAC_CLK:
		break;
	case CORE_MEMORY_POW:
		cavs_pm_runtime_core_en_memory(index);
		break;
	case CORE_HP_CLK:
		cavs_pm_runtime_core_en_hp_clk(index);
		break;
	case PM_RUNTIME_DSP:
		cavs_pm_runtime_dis_dsp_pg(index);
		break;
	default:
		break;
	}
}

void platform_pm_runtime_put(enum pm_runtime_context context, uint32_t index,
			     uint32_t flags)
{
	switch (context) {
	case PM_RUNTIME_HOST_DMA_L1:
		cavs_pm_runtime_host_dma_l1_put();
		break;
#if CONFIG_INTEL_SSP
	case SSP_CLK:
		break;
	case SSP_POW:
		cavs_pm_runtime_dis_ssp_power(index);
		break;
#endif
#if CONFIG_INTEL_DMIC
	case DMIC_CLK:
		cavs_pm_runtime_en_dmic_clk_gating(index);
		break;
	case DMIC_POW:
		cavs_pm_runtime_dis_dmic_power(index);
		break;
#endif
	case DW_DMAC_CLK:
		break;
	case CORE_MEMORY_POW:
		cavs_pm_runtime_core_dis_memory(index);
		break;
	case CORE_HP_CLK:
		cavs_pm_runtime_core_dis_hp_clk(index);
		break;
	case PM_RUNTIME_DSP:
		cavs_pm_runtime_en_dsp_pg(index);
		break;
	default:
		break;
	}
}

void platform_pm_runtime_enable(uint32_t context, uint32_t index)
{
	switch (context) {
	case PM_RUNTIME_DSP:
		cavs_pm_runtime_enable_dsp(true);
		break;
	default:
		break;
	}
}

void platform_pm_runtime_prepare_d0ix_en(uint32_t index)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&prd->lock);

	pprd->prepare_d0ix_core_mask |= BIT(index);

	k_spin_unlock(&prd->lock, key);
}

void platform_pm_runtime_prepare_d0ix_dis(uint32_t index)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&prd->lock);

	pprd->prepare_d0ix_core_mask &= ~BIT(index);

	k_spin_unlock(&prd->lock, key);
}

int platform_pm_runtime_prepare_d0ix_is_req(uint32_t index)
{
	struct pm_runtime_data *prd = pm_runtime_data_get();
	struct cavs_pm_runtime_data *pprd = prd->platform_data;

	return pprd->prepare_d0ix_core_mask & BIT(index);
}

void platform_pm_runtime_disable(uint32_t context, uint32_t index)
{
	switch (context) {
	case PM_RUNTIME_DSP:
		cavs_pm_runtime_enable_dsp(false);
		break;
	default:
		break;
	}
}

bool platform_pm_runtime_is_active(uint32_t context, uint32_t index)
{
	switch (context) {
	case PM_RUNTIME_DSP:
		return cavs_pm_runtime_is_active_dsp();
	default:
		assert(false); /* unsupported query */
		return false;
	}

}

void platform_pm_runtime_power_off(void)
{
	uint32_t hpsram_mask[PLATFORM_HPSRAM_SEGMENTS], i;
	int ret;

	/* check if DSP is busy sending IPC for 2ms */
	ret = poll_for_register_delay(IPC_HOST_BASE + IPC_DIPCIDR,
				      IPC_DIPCIDR_BUSY, 0,
				      2000);
	/* did command succeed */
	if (ret < 0)
		tr_err(&power_tr, "failed to wait for DSP sent IPC handled.");

	/* power down entire HPSRAM */
	for (i = 0; i < PLATFORM_HPSRAM_SEGMENTS; i++)
		hpsram_mask[i] = HPSRAM_MASK(i);

	power_down(true, uncache_to_cache(&hpsram_mask[0]));
}
