# Sound Detector
#
#  Generic sound detector.
#
# Pipeline Endpoints for connection are :-
#
#  (Sound Detector <-- Channel Selector) <-- Key Phrase Buffer Manager <--- Source Pipeline
#

# Include topology builder
include(`utils.m4')
include(`buffer.m4')
include(`pga.m4')
include(`ch_sel.m4')
include(`detect.m4')
include(`mixercontrol.m4')
include(`bytecontrol.m4')
include(`pipeline.m4')

#
# Controls
#


# Selector initial parameters
CONTROLBYTES_PRIV(SELECTOR_priv,
`       bytes "0x53,0x4f,0x46,0x00,0x00,0x00,0x00,0x00,'
`       0x0c,0x00,0x00,0x00,0x00,0x10,0x00,0x03,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00"'
)

# Selector Bytes control with max value of 255
C_CONTROLBYTES(SELECTOR, PIPELINE_ID,
	CONTROLBYTES_OPS(bytes, 258 binds the mixer control to bytes get/put handlers, 258, 258),
	CONTROLBYTES_EXTOPS(258 binds the mixer control to bytes get/put handlers, 258, 258),
	, , ,
	CONTROLBYTES_MAX(, 304),
	,
	SELECTOR_priv)

# Detector initial parameters for Intel KPD.
include(`detect_test_coef.m4')

# Detector Bytes control for config
C_CONTROLBYTES(Detector Config, PIPELINE_ID,
        CONTROLBYTES_OPS(bytes, 258 binds the mixer control to bytes get/put handlers, 258, 258),
        CONTROLBYTES_EXTOPS(258 binds the mixer control to bytes get/put handlers, 258, 258),
        , , ,
        CONTROLBYTES_MAX(, 304),
        ,
        DETECTOR_priv)

# Hotword Model initial parameters
CONTROLBYTES_PRIV(MODEL_priv,
`       bytes "0x53,0x4f,0x46,0x00,0x01,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x03,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'
`       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00"'
)

# Detector Bytes control for Hotword Model blob
C_CONTROLBYTES(Hotword Model, PIPELINE_ID,
        CONTROLBYTES_OPS(bytes, 258 binds the mixer control to bytes get/put handlers, 258, 258),
        CONTROLBYTES_EXTOPS(258 binds the mixer control to bytes get/put handlers, 258, 258),
        , , ,
        CONTROLBYTES_MAX(, 300000),
        ,
        MODEL_priv)

#
# Components and Buffers
#

# "Detect 0" has 2 sink period and 0 source periods
W_DETECT(0, PIPELINE_FORMAT, 0, 2, KEYWORD, N_STS(PCM_ID), SCHEDULE_CORE,
	LIST(`             ', "Detector Config", "Hotword Model"))

W_SELECTOR(0, PIPELINE_FORMAT, 2, 2, SCHEDULE_CORE,
	LIST(`		', "SELECTOR"))

# Capture Buffers
W_BUFFER(1, COMP_BUFFER_SIZE(2,
	 COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS, COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)),
	 PLATFORM_COMP_MEM_CAP)
# Capture Buffers
W_BUFFER(2, COMP_BUFFER_SIZE(2,
	 COMP_SAMPLE_SIZE(PIPELINE_FORMAT), PIPELINE_CHANNELS, COMP_PERIOD_FRAMES(PCM_MAX_RATE, SCHEDULE_PERIOD)),
	 PLATFORM_COMP_MEM_CAP)
# Virtual output widget
VIRTUAL_WIDGET(DETECT SINK PIPELINE_ID, out_drv, PIPELINE_ID)

# Pipeline
dnl W_PIPELINE(stream, deadline, priority, core, timer, platform)
W_PIPELINE(SCHED_COMP, SCHEDULE_PERIOD, SCHEDULE_PRIORITY, SCHEDULE_CORE, SCHEDULE_TIME_DOMAIN, pipe_media_schedule_plat)

#
# Pipeline Graph
#
# Detect 0 <-- B2 <-- Channel Selector 0 <-- B1

P_GRAPH(pipe-detect, PIPELINE_ID,
	LIST(`		',
	`dapm(DETECT SINK PIPELINE_ID, N_DETECT(0))',
	`dapm(N_DETECT(0), N_BUFFER(2))',
	`dapm(N_BUFFER(2), N_SELECTOR(0))',
	`dapm(N_SELECTOR(0), N_BUFFER(1))'))

#
# Pipeline Source and Sinks
#
indir(`define', concat(`PIPELINE_SINK_', PIPELINE_ID), N_BUFFER(1))
indir(`define', concat(`PIPELINE_DETECT_', PIPELINE_ID), DETECT SINK PIPELINE_ID)
