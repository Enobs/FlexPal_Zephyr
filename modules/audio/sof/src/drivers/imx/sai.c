// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright 2019 NXP
//
// Author: Daniel Baluta <daniel.baluta@nxp.com>
// Author: Jerome Laclavere <jerome.laclavere@nxp.com>
// Author: Guido Roncarolo <guido.roncarolo@nxp.com>

#include <sof/audio/component.h>
#include <rtos/bit.h>
#include <sof/drivers/edma.h>
#include <sof/drivers/sai.h>
#include <sof/lib/dai.h>
#include <sof/lib/dma.h>
#include <rtos/wait.h>
#include <sof/lib/uuid.h>
#include <ipc/dai.h>
#include <errno.h>
#include <stdint.h>

LOG_MODULE_REGISTER(sai, CONFIG_SOF_LOG_LEVEL);

/* 9302adf5-88be-4234-a0a7-dca538ef81f4 */
DECLARE_SOF_UUID("sai", sai_uuid, 0x9302adf5, 0x88be, 0x4234,
		 0xa0, 0xa7, 0xdc, 0xa5, 0x38, 0xef, 0x81, 0xf4);

DECLARE_TR_CTX(sai_tr, SOF_UUID(sai_uuid), LOG_LEVEL_INFO);

#define REG_TX_DIR 0
#define REG_RX_DIR 1

static void sai_start(struct dai *dai, int direction)
{
	dai_info(dai, "SAI: sai_start");

	int chan_idx = 0;
	uint32_t xcsr = 0U;
#ifdef CONFIG_IMX8ULP
	int fifo_offset = 0;
#endif

	if (direction == DAI_DIR_CAPTURE) {
		/* Software Reset */
		dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_CAPTURE),
				REG_SAI_CSR_SR, REG_SAI_CSR_SR);
		/* Clear SR bit to finish the reset */
		dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_CAPTURE),
				REG_SAI_CSR_SR, 0U);
		/* Check if the opposite direction is also disabled */
		xcsr = dai_read(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK));
		if (!(xcsr & REG_SAI_CSR_FRDE)) {
			/* Software Reset */
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK),
					REG_SAI_CSR_SR, REG_SAI_CSR_SR);
			/* Clear SR bit to finish the reset */
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK),
					REG_SAI_CSR_SR, 0U);
			/* Transmitter enable */
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK),
					REG_SAI_CSR_TERE, REG_SAI_CSR_TERE);
		}
	} else {
		/* Check if the opposite direction is also disabled */
		xcsr = dai_read(dai, REG_SAI_XCSR(DAI_DIR_CAPTURE));
		if (!(xcsr & REG_SAI_CSR_FRDE)) {
			/* Software Reset */
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK),
					REG_SAI_CSR_SR, REG_SAI_CSR_SR);
			/* Clear SR bit to finish the reset */
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK),
					REG_SAI_CSR_SR, 0U);
		}
	}

	/* W1C */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_FEF, 1);
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_SEF, 1);
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_WSF, 1);

	/* add one word to FIFO before TRCE is enabled */
	if (direction == DAI_DIR_PLAYBACK)
		dai_write(dai, REG_SAI_TDR0, 0x0);
	else
		dai_write(dai, REG_SAI_RDR0, 0x0);

	/* enable DMA requests */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_FRDE, REG_SAI_CSR_FRDE);

	chan_idx = BIT(0);
	/* RX3 supports capture on imx8ulp */
#ifdef CONFIG_IMX8ULP
	if (direction == DAI_DIR_CAPTURE) {
		fifo_offset = (dai_fifo(dai, DAI_DIR_CAPTURE) - dai_base(dai) - REG_SAI_RDR0) >> 2;
		chan_idx = BIT(fifo_offset);
	} else {
		fifo_offset = (dai_fifo(dai, DAI_DIR_PLAYBACK) - dai_base(dai) - REG_SAI_TDR0) >> 2;
		chan_idx = BIT(fifo_offset);
	}
#endif

	/* transmit/receive data channel enable */
	dai_update_bits(dai, REG_SAI_XCR3(direction),
			REG_SAI_CR3_TRCE_MASK, REG_SAI_CR3_TRCE(chan_idx));

	/* transmitter/receiver enable */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_TERE, REG_SAI_CSR_TERE);
}

static void sai_release(struct dai *dai, int direction)
{
	dai_info(dai, "SAI: sai_release");

	int chan_idx = 0;
#ifdef CONFIG_IMX8ULP
	int fifo_offset = 0;
#endif
	/* enable DMA requests */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_FRDE, REG_SAI_CSR_FRDE);

	chan_idx = BIT(0);
#ifdef CONFIG_IMX8ULP
	if (direction == DAI_DIR_CAPTURE) {
		fifo_offset = (dai_fifo(dai, DAI_DIR_CAPTURE) - dai_base(dai) - REG_SAI_RDR0) >> 2;
		chan_idx = BIT(fifo_offset);
	} else {
		fifo_offset = (dai_fifo(dai, DAI_DIR_PLAYBACK) - dai_base(dai) - REG_SAI_TDR0) >> 2;
		chan_idx = BIT(fifo_offset);
	}
#endif

	/* transmit/receive data channel enable */
	dai_update_bits(dai, REG_SAI_XCR3(direction),
			REG_SAI_CR3_TRCE_MASK, REG_SAI_CR3_TRCE(chan_idx));

	/* transmitter/receiver enable */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_TERE, REG_SAI_CSR_TERE);
}

static void sai_stop(struct dai *dai, int direction)
{
	dai_info(dai, "SAI: sai_stop");

	uint32_t xcsr = 0U;
	int ret = 0;

	/* Disable DMA request */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_FRDE, 0);

	/* Transmit/Receive data channel disable */
	dai_update_bits(dai, REG_SAI_XCR3(direction),
			REG_SAI_CR3_TRCE_MASK,
			REG_SAI_CR3_TRCE(0));

	/* Disable interrupts */
	dai_update_bits(dai, REG_SAI_XCSR(direction),
			REG_SAI_CSR_XIE_MASK, 0);

	/* Disable transmitter/receiver */
	if (direction == DAI_DIR_CAPTURE) {
		dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_CAPTURE), REG_SAI_CSR_TERE, 0);
		ret = poll_for_register_delay(dai_base(dai) +
					      REG_SAI_XCSR(DAI_DIR_CAPTURE),
					      REG_SAI_CSR_TERE, 0, 100);

		/* Check if the opposite direction is also disabled */
		xcsr = dai_read(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK));
		if (!(xcsr & REG_SAI_CSR_FRDE)) {
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK), REG_SAI_CSR_TERE, 0);
			ret = poll_for_register_delay(dai_base(dai) +
						      REG_SAI_XCSR(DAI_DIR_PLAYBACK),
						      REG_SAI_CSR_TERE, 0, 100);
		}
	} else {
		/* Check if the opposite direction is also disabled */
		xcsr = dai_read(dai, REG_SAI_XCSR(DAI_DIR_CAPTURE));
		if (!(xcsr & REG_SAI_CSR_FRDE)) {
			dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK), REG_SAI_CSR_TERE, 0);
			ret = poll_for_register_delay(dai_base(dai) +
						      REG_SAI_XCSR(DAI_DIR_PLAYBACK),
						      REG_SAI_CSR_TERE, 0, 100);
		}
	}

	if (ret < 0)
		dai_warn(dai, "sai: poll for register delay failed");
}

static inline int sai_set_config(struct dai *dai, struct ipc_config_dai *common_config,
				 const void *spec_config)
{
	dai_info(dai, "SAI: sai_set_config");
	const struct sof_ipc_dai_config *config = spec_config;
	uint32_t val_cr2 = 0, val_cr4 = 0, val_cr5 = 0;
	uint32_t mask_cr2 = 0, mask_cr4 = 0, mask_cr5 = 0;
	struct sai_pdata *sai = dai_get_drvdata(dai);
	/* TODO: this value will be provided by config */
#ifndef CONFIG_IMX8ULP
	uint32_t sywd = 32;
	uint32_t twm = ~(BIT(0) | BIT(1));
	uint32_t clk_div = SAI_CLOCK_DIV;
#else
	uint32_t sywd = 16;
	uint32_t twm = ~BIT(0);
	uint32_t clk_div = config->sai.fsync_rate == 8000 ? SAI_CLOCK_DIV : SAI_CLOCK_DIV_16K;
#endif

	sai->config = *config;
	sai->params = config->sai;

	val_cr4 |= REG_SAI_CR4_MF;

	switch (config->format & SOF_DAI_FMT_FORMAT_MASK) {
	case SOF_DAI_FMT_I2S:
		/*
		 * Frame low, 1clk before data, one word length for frame sync,
		 * frame sync starts one serial clock cycle earlier,
		 * that is, together with the last bit of the previous
		 * data word.
		 */
#ifdef CONFIG_IMX8ULP
		val_cr4 |= REG_SAI_CR4_FSE;
#else
		val_cr2 |= REG_SAI_CR2_BCP;
		val_cr4 |= REG_SAI_CR4_FSE | REG_SAI_CR4_FSP;
		val_cr4 |= REG_SAI_CR4_SYWD(sywd);
#endif
		break;
	case SOF_DAI_FMT_LEFT_J:
		/*
		 * Frame high, one word length for frame sync,
		 * frame sync asserts with the first bit of the frame.
		 */
		val_cr2 |= REG_SAI_CR2_BCP;
		val_cr4 |= REG_SAI_CR4_SYWD(sywd);
		break;
	case SOF_DAI_FMT_DSP_A:
		/*
		 * Frame high, 1clk before data, one bit for frame sync,
		 * frame sync starts one serial clock cycle earlier,
		 * that is, together with the last bit of the previous
		 * data word.
		 */
		val_cr2 |= REG_SAI_CR2_BCP;
		val_cr4 |= REG_SAI_CR4_FSE;
		val_cr4 |= REG_SAI_CR4_SYWD(0U);
		break;
	case SOF_DAI_FMT_DSP_B:
		/*
		 * Frame high, one bit for frame sync,
		 * frame sync asserts with the first bit of the frame.
		 */
		val_cr2 |= REG_SAI_CR2_BCP;
		val_cr4 |= REG_SAI_CR4_SYWD(0U);
		break;
	case SOF_DAI_FMT_PDM:
		val_cr2 |= REG_SAI_CR2_BCP;
		val_cr4 &= ~REG_SAI_CR4_MF;
		break;
	case SOF_DAI_FMT_RIGHT_J:
		val_cr4 |= REG_SAI_CR4_SYWD(sywd);
		break;
	default:
		return -EINVAL;
	}

	/* DAI clock inversion */
	switch (config->format & SOF_DAI_FMT_INV_MASK) {
	case SOF_DAI_FMT_IB_IF:
		/* Invert both clocks */
		val_cr2 ^= REG_SAI_CR2_BCP;
		val_cr4 ^= REG_SAI_CR4_FSP;
		break;
	case SOF_DAI_FMT_IB_NF:
		/* Invert bit clock */
		val_cr2 ^= REG_SAI_CR2_BCP;
		break;
	case SOF_DAI_FMT_NB_IF:
		/* Invert frame clock */
		val_cr4 ^= REG_SAI_CR4_FSP;
		break;
	case SOF_DAI_FMT_NB_NF:
		/* Nothing to do for both normal cases */
		break;
	default:
		return -EINVAL;
	}

	/* DAI clock provider masks */
	switch (config->format & SOF_DAI_FMT_CLOCK_PROVIDER_MASK) {
	case SOF_DAI_FMT_CBC_CFC:
		dai_info(dai, "SAI: codec is consumer");
		val_cr2 |= REG_SAI_CR2_MSEL_MCLK1;
		val_cr2 |= REG_SAI_CR2_BCD_MSTR;
		val_cr2 |= clk_div; /* TODO: determine dynamically.*/
		val_cr4 |= REG_SAI_CR4_FSD_MSTR;
		break;
	case SOF_DAI_FMT_CBP_CFP:
		dai_info(dai, "SAI: codec is provider");
		/*
		 * fields CR2_DIV and CR2_MSEL not relevant in consumer mode.
		 * fields CR2_BCD and CR4_MFSD already at 0
		 */
		break;
	case SOF_DAI_FMT_CBC_CFP:
		val_cr2 |= REG_SAI_CR2_BCD_MSTR;
		val_cr2 |= clk_div; /* TODO: determine dynamically.*/
		break;
	case SOF_DAI_FMT_CBP_CFC:
		val_cr4 |= REG_SAI_CR4_FSD_MSTR;
		val_cr2 |= clk_div; /* TODO: determine dynamically.*/
		break;
	default:
		return -EINVAL;
	}

	/* TODO: set number of slots from config */
	val_cr4 |= REG_SAI_CR4_FRSZ(SAI_TDM_SLOTS);
	val_cr4 |= REG_SAI_CR4_CHMOD;

	val_cr5 |= REG_SAI_CR5_WNW(sywd) | REG_SAI_CR5_W0W(sywd) |
			REG_SAI_CR5_FBT(sywd);

	mask_cr2  = REG_SAI_CR2_BCP | REG_SAI_CR2_BCD_MSTR |
			REG_SAI_CR2_MSEL_MASK | REG_SAI_CR2_DIV_MASK;

	mask_cr4  = REG_SAI_CR4_MF | REG_SAI_CR4_FSE |
			REG_SAI_CR4_FSP | REG_SAI_CR4_FSD_MSTR |
			REG_SAI_CR4_FRSZ_MASK | REG_SAI_CR4_SYWD_MASK |
			REG_SAI_CR4_CHMOD_MASK;

	mask_cr5  = REG_SAI_CR5_WNW_MASK | REG_SAI_CR5_W0W_MASK |
			REG_SAI_CR5_FBT_MASK;

	dai_update_bits(dai, REG_SAI_XCR1(REG_TX_DIR), REG_SAI_CR1_RFW_MASK,
			dai->plat_data.fifo[REG_TX_DIR].watermark);
	dai_update_bits(dai, REG_SAI_XCR2(REG_TX_DIR), mask_cr2, val_cr2);
	dai_update_bits(dai, REG_SAI_XCR4(REG_TX_DIR), mask_cr4, val_cr4);
	dai_update_bits(dai, REG_SAI_XCR5(REG_TX_DIR), mask_cr5, val_cr5);
	/* turn on (set to zero) stereo slot */
	dai_update_bits(dai, REG_SAI_XMR(REG_TX_DIR),  REG_SAI_XMR_MASK,
		       twm);

	val_cr2 |= REG_SAI_CR2_SYNC;
	mask_cr2 |= REG_SAI_CR2_SYNC_MASK;

	dai_update_bits(dai, REG_SAI_XCR1(REG_RX_DIR), REG_SAI_CR1_RFW_MASK,
			dai->plat_data.fifo[REG_RX_DIR].watermark);
	dai_update_bits(dai, REG_SAI_XCR2(REG_RX_DIR), mask_cr2, val_cr2);
	dai_update_bits(dai, REG_SAI_XCR4(REG_RX_DIR), mask_cr4, val_cr4);
	dai_update_bits(dai, REG_SAI_XCR5(REG_RX_DIR), mask_cr5, val_cr5);
	/* turn on (set to zero) stereo slot */
	dai_update_bits(dai, REG_SAI_XMR(REG_RX_DIR), REG_SAI_XMR_MASK,
			twm);

#ifdef CONFIG_IMX8M
	/*
	 * For i.MX8MP, MCLK is bound with TX enable bit.
	 * Therefore, enable transmitter to output MCLK
	 */
	dai_update_bits(dai, REG_SAI_XCSR(DAI_DIR_PLAYBACK),
			REG_SAI_CSR_TERE, REG_SAI_CSR_TERE);
	dai_update_bits(dai, REG_SAI_MCTL, REG_SAI_MCTL_MCLK_EN,
			REG_SAI_MCTL_MCLK_EN);
#endif

	return 0;
}

static int sai_trigger(struct dai *dai, int cmd, int direction)
{
	dai_info(dai, "SAI: sai_trigger");

	switch (cmd) {
	case COMP_TRIGGER_START:
		sai_start(dai, direction);
		break;
	case COMP_TRIGGER_RELEASE:
		sai_release(dai, direction);
		break;
	case COMP_TRIGGER_STOP:
	case COMP_TRIGGER_PAUSE:
		sai_stop(dai, direction);
		break;
	case COMP_TRIGGER_PRE_START:
	case COMP_TRIGGER_PRE_RELEASE:
		break;
	default:
		dai_err(dai, "SAI: invalid trigger cmd %d", cmd);
		break;
	}
	return 0;
}

static int sai_probe(struct dai *dai)
{
	struct sai_pdata *sai;

	dai_info(dai, "SAI: sai_probe");

	/* allocate private data */
	sai = rzalloc(SOF_MEM_ZONE_RUNTIME_SHARED, 0, SOF_MEM_CAPS_RAM, sizeof(*sai));
	if (!sai) {
		dai_err(dai, "sai_probe(): alloc failed");
		return -ENOMEM;
	}
	dai_set_drvdata(dai, sai);

	/* Software Reset for both Tx and Rx */
	dai_update_bits(dai, REG_SAI_TCSR, REG_SAI_CSR_SR, REG_SAI_CSR_SR);
	dai_update_bits(dai, REG_SAI_RCSR, REG_SAI_CSR_SR, REG_SAI_CSR_SR);

	/* Clear SR bit to finish the reset */
	dai_update_bits(dai, REG_SAI_TCSR, REG_SAI_CSR_SR, 0U);
	dai_update_bits(dai, REG_SAI_RCSR, REG_SAI_CSR_SR, 0U);

	/* Reset all Tx register values */
	dai_write(dai, REG_SAI_TCR2, 0U);
	dai_write(dai, REG_SAI_TCR3, 0U);
	dai_write(dai, REG_SAI_TCR4, 0U);
	dai_write(dai, REG_SAI_TCR5, 0U);
	dai_write(dai, REG_SAI_TMR,  0U);

	/* Reset all Rx register values */
	dai_write(dai, REG_SAI_RCR2, 0U);
	dai_write(dai, REG_SAI_RCR3, 0U);
	dai_write(dai, REG_SAI_RCR4, 0U);
	dai_write(dai, REG_SAI_RCR5, 0U);
	dai_write(dai, REG_SAI_RMR,  0U);

	return 0;
}

static int sai_remove(struct dai *dai)
{
	struct sai_pdata *sai = dai_get_drvdata(dai);

	dai_info(dai, "sai_remove()");

	rfree(sai);
	dai_set_drvdata(dai, NULL);

	return 0;
}

static int sai_get_handshake(struct dai *dai, int direction, int stream_id)
{
	return dai->plat_data.fifo[direction].handshake;
}

static int sai_get_fifo(struct dai *dai, int direction, int stream_id)
{
	switch (direction) {
	case DAI_DIR_PLAYBACK:
	case DAI_DIR_CAPTURE:
		return dai_fifo(dai, direction); /* stream_id is unused */
	default:
		dai_err(dai, "sai_get_fifo(): Invalid direction");
		return -EINVAL;
	}
}

static int sai_get_fifo_depth(struct dai *dai, int direction)
{
	switch (direction) {
	case DAI_DIR_PLAYBACK:
	case DAI_DIR_CAPTURE:
		return dai->plat_data.fifo[direction].depth;
	default:
		dai_err(dai, "esai_get_fifo_depth(): Invalid direction");
		return -EINVAL;
	}
}

static int sai_get_hw_params(struct dai *dai,
			     struct sof_ipc_stream_params *params,
			     int dir)
{
	struct sai_pdata *sai = dai_get_drvdata(dai);

	/* SAI only currently supports these parameters */
	params->rate = sai->params.fsync_rate;
#ifdef CONFIG_IMX8ULP
	params->channels = 1;
#else
	params->channels = 2;
#endif
	params->buffer_fmt = 0;
	params->frame_fmt = SOF_IPC_FRAME_S32_LE;

	return 0;
}

const struct dai_driver sai_driver = {
	.type = SOF_DAI_IMX_SAI,
	.uid = SOF_UUID(sai_uuid),
	.tctx = &sai_tr,
	.dma_dev = DMA_DEV_SAI,
	.ops = {
		.trigger		= sai_trigger,
		.set_config		= sai_set_config,
		.probe			= sai_probe,
		.remove			= sai_remove,
		.get_handshake		= sai_get_handshake,
		.get_fifo		= sai_get_fifo,
		.get_fifo_depth		= sai_get_fifo_depth,
		.get_hw_params		= sai_get_hw_params,
	},
};
