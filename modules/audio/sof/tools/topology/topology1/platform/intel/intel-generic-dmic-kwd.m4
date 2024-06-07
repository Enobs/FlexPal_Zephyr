#
# Topology for digital microphones array
#

include(`platform/intel/dmic.m4')

# define default PCM names
ifdef(`DMIC_48k_PCM_NAME',`',
`define(DMIC_48k_PCM_NAME, `DMIC')')
ifdef(`DMIC_16k_PCM_NAME',`',
`define(DMIC_16k_PCM_NAME, `DMIC16kHz')')

# variable that need to be defined in upper m4
ifdef(`CHANNELS',`',`fatal_error(note: Need to define channel number for intel-generic-dmic-kwd
)')
ifdef(`KFBM_TYPE',`',`fatal_error(note: Need to define kfbm type for intel-generic-dmic-kwd, available type: vol-kfbm/kfbm
)')
ifdef(`DMIC_PCM_48k_ID',`',`fatal_error(note: Need to define dmic48k pcm id for intel-generic-dmic-kwd
)')
ifdef(`DMIC_PIPELINE_48k_ID',`',`fatal_error(note: Need to define dmic48k pipeline id for intel-generic-dmic-kwd
)')
ifdef(`DMIC_DAI_LINK_48k_ID',`',`fatal_error(note: Need to define dmic48k dai id for intel-generic-dmic-kwd
)')

ifdef(`DMIC_PCM_16k_ID',`',`fatal_error(note: Need to define dmic16k pcm id for intel-generic-dmic-kwd
)')
ifdef(`DMIC_PIPELINE_16k_ID',`',`fatal_error(note: Need to define dmic16k pipeline id for intel-generic-dmic-kwd
)')
ifdef(`DMIC_PIPELINE_KWD_ID',`',`fatal_error(note: Need to define kwd pipeline id for intel-generic-dmic-kwd
)')
ifdef(`DMIC_DAI_LINK_16k_ID',`',`fatal_error(note: Need to define dmic16k dai id for intel-generic-dmic-kwd
)')

# define(KWD_PIPE_SCH_DEADLINE_US, 20000)
ifdef(`KWD_PIPE_SCH_DEADLINE_US',`',`fatal_error(note: Need to define schedule for intel-generic-dmic-kwd
)')

# define(DMIC_DAI_LINK_48k_NAME, `dmic01')
ifdef(`DMIC_DAI_LINK_48k_NAME',`',define(DMIC_DAI_LINK_48k_NAME, `dmic01'))

# define(DMIC_PIPELINE_48k_CORE_ID, 0)
ifdef(`DMIC_PIPELINE_48k_CORE_ID',`',define(DMIC_PIPELINE_48k_CORE_ID, 0))

# define(DMIC_DAI_LINK_16k_NAME, `dmic16k')
ifdef(`DMIC_DAI_LINK_16k_NAME',`',define(DMIC_DAI_LINK_16k_NAME, `dmic16k'))

# DMICPROC is set by makefile, available type: passthrough/eq-iir-volume
ifdef(`DMICPROC', `', `define(DMICPROC, passthrough)')

dnl Unless explicitly specified, dmic period at 48k is 1ms
ifdef(`DMIC_48k_PERIOD_US', `', `define(`DMIC_48k_PERIOD_US', 1000)')

# define(DMIC_DAI_LINK_16k_PDM, `STEREO_PDM0') define the PDM port, default is STEREO_PDM0
ifdef(`DMIC_DAI_LINK_16k_PDM',`',`define(DMIC_DAI_LINK_16k_PDM, `STEREO_PDM0')')

# define(DETECTOR_TYPE, `google-hotword-detect') define the detector type, default is the test detector
ifdef(`DETECTOR_TYPE',`',define(DETECTOR_TYPE, `detect'))

#
# Define the pipelines
#

dnl PIPELINE_PCM_ADD(pipeline,
dnl     pipe id, pcm, max channels, format,
dnl     period, priority, core,
dnl     pcm_min_rate, pcm_max_rate, pipeline_rate,
dnl     time_domain, sched_comp)

# Passthrough capture pipeline using max channels defined by CHANNELS.

# Set 1000us deadline with priority 0 on core 0
ifdef(`DMICPROC_FILTER1', `define(PIPELINE_FILTER1, DMICPROC_FILTER1)', `undefine(`PIPELINE_FILTER1')')
ifdef(`DMICPROC_FILTER2', `define(PIPELINE_FILTER2, DMICPROC_FILTER2)', `undefine(`PIPELINE_FILTER2')')
define(`PGA_NAME', Dmic0)

PIPELINE_PCM_ADD(sof/pipe-`DMICPROC'-capture.m4,
        DMIC_PIPELINE_48k_ID, DMIC_PCM_48k_ID, CHANNELS, s32le,
        DMIC_48k_PERIOD_US, 0, DMIC_PIPELINE_48k_CORE_ID, 48000, 48000, 48000)
undefine(`PGA_NAME')
undefine(`PIPELINE_FILTER1')
undefine(`PIPELINE_FILTER2')
undefine(`DMICPROC')

#
# KWD configuration
#

# Passthrough capture pipeline 7 on PCM 7 using max 2 channels.
# Schedule 20000us deadline with priority 0 on core 0
PIPELINE_PCM_DAI_ADD(sof/pipe-KFBM_TYPE-capture.m4,
        DMIC_PIPELINE_16k_ID, DMIC_PCM_16k_ID, 2, s32le,
        KWD_PIPE_SCH_DEADLINE_US, 0, 0, DMIC, 1, s32le, 3,
        16000, 16000, 16000)



#
# DAIs configuration
#

dnl DAI_ADD(pipeline,
dnl     pipe id, dai type, dai_index, dai_be,
dnl     buffer, periods, format,
dnl     deadline, priority, core, time_domain)

# capture DAI is DMIC 0 using 2 periods
# Buffers use s32le format, 1000us deadline with priority 0 on core 0
DAI_ADD(sof/pipe-dai-capture.m4,
        DMIC_PIPELINE_48k_ID, DMIC, 0, DMIC_DAI_LINK_48k_NAME,
        concat(`PIPELINE_SINK_', DMIC_PIPELINE_48k_ID), 2, s32le,
        DMIC_48k_PERIOD_US, 0, DMIC_PIPELINE_48k_CORE_ID, SCHEDULE_TIME_DOMAIN_TIMER)

# capture DAI is DMIC 1 using 3 periods
# Buffers use s32le format, with 320 frame per 20000us on core 0 with priority 0
DAI_ADD(sof/pipe-dai-capture.m4,
        DMIC_PIPELINE_16k_ID, DMIC, 1, DMIC_DAI_LINK_16k_NAME,
        `PIPELINE_SINK_'DMIC_PIPELINE_16k_ID, 3, s32le,
        KWD_PIPE_SCH_DEADLINE_US, 0, 0, SCHEDULE_TIME_DOMAIN_TIMER)

dnl PCM_DUPLEX_ADD(name, pcm_id, playback, capture)
dnl PCM_CAPTURE_ADD(name, pipeline, capture)
PCM_CAPTURE_ADD(DMIC_48k_PCM_NAME, DMIC_PCM_48k_ID, concat(`PIPELINE_PCM_', DMIC_PIPELINE_48k_ID))

# keyword detector pipe
dnl PIPELINE_PCM_ADD(pipeline,
dnl     pipe id, pcm, max channels, format,
dnl     period, priority, core,
dnl     pcm_min_rate, pcm_max_rate, pipeline_rate,
dnl     time_domain, sched_comp, dynamic)
PIPELINE_PCM_ADD(sof/pipe-DETECTOR_TYPE.m4,
        DMIC_PIPELINE_KWD_ID, DMIC_PCM_16k_ID, 2, s24le,
        KWD_PIPE_SCH_DEADLINE_US, 1, 0,
        16000, 16000, 16000,
		SCHEDULE_TIME_DOMAIN_TIMER,
		`PIPELINE_SCHED_COMP_'DMIC_PIPELINE_16k_ID)

# Connect pipelines together
SectionGraph."pipe-sof-generic-keyword-detect" {
        index "0"

        lines [
                # keyword detect
                dapm(`PIPELINE_SINK_'DMIC_PIPELINE_KWD_ID, `PIPELINE_SOURCE_'DMIC_PIPELINE_16k_ID)
                dapm(`PIPELINE_PCM_'DMIC_PIPELINE_16k_ID, `PIPELINE_DETECT_'DMIC_PIPELINE_KWD_ID)
        ]
}

#
# BE configurations - overrides config in ACPI if present
#

dnl DAI_CONFIG(type, dai_index, link_id, name, ssp_config/dmic_config)
ifelse(CHANNELS, 4,
`DAI_CONFIG(DMIC, 0, DMIC_DAI_LINK_48k_ID, DMIC_DAI_LINK_48k_NAME,
                DMIC_CONFIG(1, 2400000, 4800000, 40, 60, 48000,
                DMIC_WORD_LENGTH(s32le), 200, DMIC, 0,
                PDM_CONFIG(DMIC, 0, FOUR_CH_PDM0_PDM1)))',
`DAI_CONFIG(DMIC, 0, DMIC_DAI_LINK_48k_ID, DMIC_DAI_LINK_48k_NAME,
                DMIC_CONFIG(1, 2400000, 4800000, 40, 60, 48000,
                DMIC_WORD_LENGTH(s32le), 200, DMIC, 0,
                PDM_CONFIG(DMIC, 0, STEREO_PDM0)))')

# dmic16k (ID: 2)
DAI_CONFIG(DMIC, 1, DMIC_DAI_LINK_16k_ID, DMIC_DAI_LINK_16k_NAME,
                DMIC_CONFIG(1, 2400000, 4800000, 40, 60, 16000,
                DMIC_WORD_LENGTH(s32le), 400, DMIC, 1,
                PDM_CONFIG(DMIC, 1, DMIC_DAI_LINK_16k_PDM)))
