# Capture EQ Pipeline and PCM, 48 kHz
#
# Pipeline Endpoints for connection are :-
#
#  host PCM_C <-- B0 <-- Volume <-- B1 <-- EQIIR <-- B2 <-- TDFB <-- B3 <-- sink DAI0

# Include topology builder
include(`utils.m4')
include(`buffer.m4')
include(`pcm.m4')
include(`pga.m4')
include(`dai.m4')
include(`pipeline.m4')
include(`bytecontrol.m4')
include(`mixercontrol.m4')
include(`enumcontrol.m4')
include(`tdfb.m4')
include(`eq_iir.m4')

ifdef(`PGA_NAME', `', `define(PGA_NAME, N_PGA(0))')
define(`CONTROL_NAME_VOLUME', Capture Volume)
define(`CONTROL_NAME_SWITCH', Capture Switch)

#
# Controls
#

# defines with pipeline ID appended for unique names
include(`tdfb_defines.m4')

# Define filter. A passthrough is set by default.
ifdef(`PIPELINE_FILTER1', , `define(PIPELINE_FILTER1, `tdfb/coef_line2_pass.m4')')
include(PIPELINE_FILTER1)

# Include defines for TDBF controls
include(`tdfb_controls.m4')

# Volume Mixer control with max value of 32
define(`CONTROL_NAME', `CONTROL_NAME_VOLUME')
C_CONTROLMIXER(Capture Volume, PIPELINE_ID,
	CONTROLMIXER_OPS(volsw,
		256 binds the mixer control to volume get/put handlers,
		256, 256),
	CONTROLMIXER_MAX(, 70),
	false,
	CONTROLMIXER_TLV(TLV 80 steps from -50dB to +20dB for 1dB, vtlv_m50s1),
	Channel register and shift for Front Left/Right,
	VOLUME_CHANNEL_MAP)

undefine(`CONTROL_NAME')

# Switch type Mixer Control with max value of 1
define(`CONTROL_NAME', `CONTROL_NAME_SWITCH')
C_CONTROLMIXER(Capture Switch, PIPELINE_ID,
	CONTROLMIXER_OPS(volsw, 259 binds the mixer control to switch get/put handlers, 259, 259),
	CONTROLMIXER_MAX(max 1 indicates switch type control, 1),
	false,
	,
	Channel register and shift for Front Left/Right,
	SWITCH_CHANNEL_MAP,
	"1", "1")
undefine(`CONTROL_NAME')

# Volume Configuration
define(DEF_PGA_TOKENS, concat(`pga_tokens_', PIPELINE_ID))
define(DEF_PGA_CONF, concat(`pga_conf_', PIPELINE_ID))

W_VENDORTUPLES(DEF_PGA_TOKENS, sof_volume_tokens,
LIST(`		', `SOF_TKN_VOLUME_RAMP_STEP_TYPE	"0"'
     `		', `SOF_TKN_VOLUME_RAMP_STEP_MS		"250"'))

W_DATA(DEF_PGA_CONF, DEF_PGA_TOKENS)

define(DEF_EQIIR_COEF, concat(`eqiir_coef_', PIPELINE_ID))
define(DEF_EQIIR_PRIV, concat(`eqiir_priv_', PIPELINE_ID))

# By default, use 40 Hz highpass response with +0 dB gain for 48khz
# TODO: need to implement middle level macro handler per pipeline
ifdef(`PIPELINE_FILTER2', , `define(PIPELINE_FILTER2, eq_iir_coef_highpass_40hz_0db_48khz.m4)')
include(PIPELINE_FILTER2)

# EQ Bytes control with max value of 255
C_CONTROLBYTES(DEF_EQIIR_COEF, PIPELINE_ID,
	CONTROLBYTES_OPS(bytes,
		258 binds the mixer control to bytes get/put handlers,
		258, 258),
	CONTROLBYTES_EXTOPS(
		258 binds the mixer control to bytes get/put handlers,
		258, 258),
	, , ,
	CONTROLBYTES_MAX(, 1024),
	,
	DEF_EQIIR_PRIV)

#
# Components and Buffers
#

# Host "TDFB Capture" PCM
# with 0 sink and 2 source periods
W_PCM_CAPTURE(PCM_ID, TDFB Capture, 0, 2, SCHEDULE_CORE)

# "Volume" has 2 source and 2 sink periods
W_PGA(0, PIPELINE_FORMAT, 2, 2, DEF_PGA_CONF, SCHEDULE_CORE,
	LIST(`		', "CONTROL_NAME_VOLUME",
	"CONTROL_NAME_SWITCH"))

# "EQ 0" has 2 sink period and 2 source periods
W_EQ_IIR(0, PIPELINE_FORMAT, 2, 2, SCHEDULE_CORE,
	LIST(`		', "DEF_EQIIR_COEF"))

# "TDFB 0" has 2 sink period and x source periods
# Note that the controls will receive index values 0, 1, 2 in the order below
W_TDFB(0, PIPELINE_FORMAT, 2, DAI_PERIODS, SCHEDULE_CORE,
	LIST(`		', "DEF_TDFB_BEAM"),
	LIST(`		', "DEF_TDFB_DIRECTION"),
	LIST(`		', "DEF_TDFB_AZIMUTH"),
	LIST(`		', "DEF_TDFB_AZIMUTH_ESTIMATE"),
	LIST(`		', "DEF_TDFB_BYTES"))

# Capture Buffers
W_BUFFER(0, COMP_BUFFER_SIZE(2,
	COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS,
	COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)), PLATFORM_PASS_MEM_CAP)

W_BUFFER(1, COMP_BUFFER_SIZE(2,
	COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS,
	COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)), PLATFORM_PASS_MEM_CAP)

W_BUFFER(2, COMP_BUFFER_SIZE(2,
	COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS,
	COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)), PLATFORM_PASS_MEM_CAP)

W_BUFFER(3, COMP_BUFFER_SIZE(DAI_PERIODS,
	COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS,
	COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)), PLATFORM_PASS_MEM_CAP)

#
# Pipeline Graph
#
#  host PCM_C <-- B0 <-- Volume <-- B1 <-- EQIIR <-- B2 <-- TDFB <-- B3 <-- sink DAI0

P_GRAPH(pipe-tdfb-eq-iir-volume-capture, PIPELINE_ID,
	LIST(`		',
	`dapm(N_PCMC(PCM_ID), N_BUFFER(0))',
	`dapm(N_BUFFER(0), PGA_NAME)',
	`dapm(PGA_NAME, N_BUFFER(1))',
	`dapm(N_BUFFER(1), N_EQ_IIR(0))',
	`dapm(N_EQ_IIR(0), N_BUFFER(2))',
	`dapm(N_BUFFER(2), N_TDFB(0))',
	`dapm(N_TDFB(0), N_BUFFER(3))'))

undefine(`PGA_NAME')
undefine(`CONTROL_NAME_VOLUME')
undefine(`CONTROL_NAME_SWITCH')

#
# Pipeline Source and Sinks
#
indir(`define', concat(`PIPELINE_SINK_', PIPELINE_ID), N_BUFFER(3))
indir(`define', concat(`PIPELINE_PCM_', PIPELINE_ID), TDFB Capture PCM_ID)

#
# PCM Configuration
#

PCM_CAPABILITIES(TDFB Capture PCM_ID, CAPABILITY_FORMAT_NAME(PIPELINE_FORMAT), PCM_MIN_RATE,
	PCM_MAX_RATE, PIPELINE_CHANNELS, PIPELINE_CHANNELS,
	2, 16, 192, 16384, 65536, 65536)

undefine(`DEF_PGA_TOKENS')
undefine(`DEF_PGA_CONF')
undefine(`DEF_EQIIR_COEF')
undefine(`DEF_EQIIR_PRIV')
include(`tdfb_undefines.m4')
