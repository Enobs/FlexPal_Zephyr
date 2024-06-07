#
# Topology for Cometlake with rt5682 codec.
#

# Include topology builder
include(`utils.m4')
include(`dai.m4')
include(`pipeline.m4')
include(`ssp.m4')
include(`hda.m4')

# Include TLV library
include(`common/tlv.m4')

# Include Token library
include(`sof/tokens.m4')

# Include Platform specific DSP configuration
include(`platform/intel/'PLATFORM`.m4')

DEBUG_START

# if XPROC is not defined, define with default pipe
ifdef(`HSMICPROC', , `define(HSMICPROC, volume)')
ifdef(`HSEARPROC', , `define(HSEARPROC, volume)')
ifdef(`DMICPROC', , `define(DMICPROC, passthrough)')
ifdef(`DMIC16KPROC', , `define(DMIC16KPROC, passthrough)')

#
# Define the pipelines
#
`# PCM0 ---> 'HSEARPROC` ----> SSP(SSP_INDEX, BE link 0)'
`# PCM0 <--- 'HSMICPROC` <---- SSP(SSP_INDEX, BE link 0)'
`# PCM1 <--- 'DMICPROC` <----- DMIC01 (dmic0 capture, , BE link 1)'
# PCM2 ----> volume -----> iDisp1 (HDMI/DP playback, BE link 3)
# PCM3 ----> volume -----> iDisp2 (HDMI/DP playback, BE link 4)
# PCM4 ----> volume -----> iDisp3 (HDMI/DP playback, BE link 5)
`# PCM8 <---- 'DMIC16KPROC` <----- DMIC16k (dmic16k, BE link 2)'
#

dnl PIPELINE_PCM_ADD(pipeline,
dnl     pipe id, pcm, max channels, format,
dnl     period, priority, core,
dnl     pcm_min_rate, pcm_max_rate, pipeline_rate,
dnl     time_domain, sched_comp)

# Low Latency playback pipeline 1 on PCM 0 using max 2 channels of s24le.
# 1000us deadline with priority 0 on core 0
PIPELINE_PCM_ADD(sof/pipe-`HSEARPROC'-playback.m4,
	1, 0, 2, s24le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency capture pipeline 2 on PCM 0 using max 2 channels of s24le.
# 1000us deadline with priority 0 on core 0
PIPELINE_PCM_ADD(sof/pipe-`HSMICPROC'-capture.m4,
	2, 0, 2, s24le,
	1000, 0, 0,
	48000, 48000, 48000)

# Passthrough capture pipeline 3 on PCM 1 using max 4 channels.
# 1000us deadline with priority 0 on core 0
ifdef(`DMICPROC_FILTER1', `define(PIPELINE_FILTER1, DMICPROC_FILTER1)', `undefine(`PIPELINE_FILTER1')')
ifdef(`DMICPROC_FILTER2', `define(PIPELINE_FILTER2, DMICPROC_FILTER2)', `undefine(`PIPELINE_FILTER2')')
define(`PGA_NAME', Dmic0)

PIPELINE_PCM_ADD(sof/pipe-`DMICPROC'-capture.m4,
	3, 1, 4, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

undefine(`PGA_NAME')
undefine(`PIPELINE_FILTER1')
undefine(`PIPELINE_FILTER2')

# Low Latency playback pipeline 4 on PCM 2 using max 2 channels of s32le.
# 1000us deadline with priority 0 on core 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	4, 2, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency playback pipeline 5 on PCM 3 using max 2 channels of s32le.
# 1000us deadline with priority 0 on core 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	5, 3, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Low Latency playback pipeline 6 on PCM 4 using max 2 channels of s32le.
# 1000us deadline with priority 0 on core 0
PIPELINE_PCM_ADD(sof/pipe-volume-playback.m4,
	6, 4, 2, s32le,
	1000, 0, 0,
	48000, 48000, 48000)

# Passthrough capture pipeline 7 on PCM 5 using max 2 channels.
# Schedule 1000us deadline with priority 0 on core 0
ifdef(`DMIC16KPROC_FILTER1', `define(PIPELINE_FILTER1, DMIC16KPROC_FILTER1)', `undefine(`PIPELINE_FILTER1')')
ifdef(`DMIC16KPROC_FILTER2', `define(PIPELINE_FILTER2, DMIC16KPROC_FILTER2)', `undefine(`PIPELINE_FILTER2')')
define(`PGA_NAME', Dmic1)

PIPELINE_PCM_ADD(sof/pipe-`DMIC16KPROC'-capture-16khz.m4,
	8, 8, 2, s24le,
	1000, 0, 0,
	16000, 16000, 16000)

undefine(`PGA_NAME')
undefine(`PIPELINE_FILTER1')
undefine(`PIPELINE_FILTER2')

#
# DAIs configuration
#

dnl DAI_ADD(pipeline,
dnl     pipe id, dai type, dai_index, dai_be,
dnl     buffer, periods, format,
dnl     deadline, priority, core, time_domain)

# playback DAI is SSP(SPP_INDEX) using 2 periods
# Buffers use s24le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-playback.m4,
	1, SSP, SSP_INDEX, SSP_NAME,
	PIPELINE_SOURCE_1, 2, s24le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# capture DAI is SSP(SSP_INDEX) using 2 periods
# Buffers use s24le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-capture.m4,
	2, SSP,SSP_INDEX, SSP_NAME,
	PIPELINE_SINK_2, 2, s24le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# capture DAI is DMIC01 using 2 periods
# Buffers use s32le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-capture.m4,
	3, DMIC, 0, dmic01,
	PIPELINE_SINK_3, 2, s32le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# playback DAI is iDisp1 using 2 periods
# Buffers use s32le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-playback.m4,
	4, HDA, 0, iDisp1,
	PIPELINE_SOURCE_4, 2, s32le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# playback DAI is iDisp2 using 2 periods
# Buffers use s32le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-playback.m4,
	5, HDA, 1, iDisp2,
	PIPELINE_SOURCE_5, 2, s32le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# playback DAI is iDisp3 using 2 periods
# Buffers use s32le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-playback.m4,
	6, HDA, 2, iDisp3,
	PIPELINE_SOURCE_6, 2, s32le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

# capture DAI is DMIC16k using 2 periods
# Buffers use s32le format, with 16 frame per 1000us on core 0 with priority 0
DAI_ADD(sof/pipe-dai-capture.m4,
	8, DMIC, 1, dmic16k,
	PIPELINE_SINK_8, 2, s32le,
	1000, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)


# PCM Low Latency, id 0
dnl PCM_PLAYBACK_ADD(name, pcm_id, playback)
PCM_DUPLEX_ADD(Port1, 0, PIPELINE_PCM_1, PIPELINE_PCM_2)
PCM_CAPTURE_ADD(DMIC, 1, PIPELINE_PCM_3)
PCM_PLAYBACK_ADD(HDMI1, 2, PIPELINE_PCM_4)
PCM_PLAYBACK_ADD(HDMI2, 3, PIPELINE_PCM_5)
PCM_PLAYBACK_ADD(HDMI3, 4, PIPELINE_PCM_6)
PCM_CAPTURE_ADD(DMIC16kHz, 8, PIPELINE_PCM_8)

#
# BE configurations - overrides config in ACPI if present
#

#SSP SSP_INDEX (ID: 0)
DAI_CONFIG(SSP, SSP_INDEX, 0, SSP_NAME,
	SSP_CONFIG(I2S, SSP_CLOCK(mclk, SSP_MCLK_RATE, codec_mclk_in),
		      SSP_CLOCK(bclk, 2400000, codec_slave),
		      SSP_CLOCK(fsync, 48000, codec_slave),
		      SSP_TDM(2, 25, 3, 3),
		      SSP_CONFIG_DATA(SSP, SSP_INDEX, 24)))

# dmic01 (ID: 1)
DAI_CONFIG(DMIC, 0, 1, dmic01,
	   DMIC_CONFIG(1, 2400000, 4800000, 40, 60, 48000,
		DMIC_WORD_LENGTH(s32le), 400, DMIC, 0,
		PDM_CONFIG(DMIC, 0, FOUR_CH_PDM0_PDM1)))

# dmic16k (ID: 2)
DAI_CONFIG(DMIC, 1, 2, dmic16k,
	   DMIC_CONFIG(1, 2400000, 4800000, 40, 60, 16000,
		DMIC_WORD_LENGTH(s32le), 400, DMIC, 1,
		PDM_CONFIG(DMIC, 1, STEREO_PDM0)))

# 3 HDMI/DP outputs (ID: 3,4,5)
DAI_CONFIG(HDA, 0, 3, iDisp1,
	HDA_CONFIG(HDA_CONFIG_DATA(HDA, 0, 48000, 2)))
DAI_CONFIG(HDA, 1, 4, iDisp2,
	HDA_CONFIG(HDA_CONFIG_DATA(HDA, 1, 48000, 2)))
DAI_CONFIG(HDA, 2, 5, iDisp3,
	HDA_CONFIG(HDA_CONFIG_DATA(HDA, 2, 48000, 2)))

DEBUG_END
