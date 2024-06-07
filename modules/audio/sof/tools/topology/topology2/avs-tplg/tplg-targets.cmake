# SPDX-License-Identifier: BSD-3-Clause

# Array of "input-file-name;output-file-name;comma separated pre-processor variables"
set(TPLGS
# CAVS HDMI only topology with passthrough pipelines
"sof-hda-generic\;sof-hda-generic-idisp\;USE_CHAIN_DMA=true,DEEPBUFFER_FW_DMA_MS=100"
# CAVS HDA topology with mixer-based pipelines for HDA and passthrough pipelines for HDMI
"sof-hda-generic\;sof-hda-generic\;HDA_CONFIG=mix,USE_CHAIN_DMA=true,DEEPBUFFER_FW_DMA_MS=100"
# If the alsatplg plugins for NHLT are not available, the NHLT blobs will not be added to the
# topologies below.
"sof-hda-generic\;sof-hda-generic-2ch\;\
HDA_CONFIG=mix,NUM_DMICS=2,PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-hda-generic-2ch.bin,USE_CHAIN_DMA=true,\
DEEPBUFFER_FW_DMA_MS=100"
"sof-hda-generic\;sof-hda-generic-4ch\;\
HDA_CONFIG=mix,NUM_DMICS=4,PDM1_MIC_A_ENABLE=1,PDM1_MIC_B_ENABLE=1,\
PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-hda-generic-4ch.bin,USE_CHAIN_DMA=true,DEEPBUFFER_FW_DMA_MS=100"
# CAVS SDW topology with passthrough pipelines
"cavs-sdw\;cavs-sdw\;DEEPBUFFER_FW_DMA_MS=100,\
PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-cavs-sdw.bin"

# IPC4 topology for TGL rt711 Headset + rt1316 Amplifier + rt714 DMIC
"cavs-sdw\;sof-tgl-rt711-rt1316-rt714\;NUM_SDW_AMPS=2,SDW_DMIC=1,\
PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-tgl-rt711-rt1316-rt714.bin"

"cavs-sdw\;sof-adl-rt711-l0-rt1316-l12-rt714-l3\;NUM_SDW_AMPS=2,SDW_DMIC=1"

# IPC4 topology for TGL rt711 Headset + rt1308 Amplifier + rt715 DMIC
"cavs-sdw\;sof-tgl-rt715-rt711-rt1308-mono\;NUM_SDW_AMPS=1,SDW_DMIC=1,\
SDW_JACK_OUT_STREAM=SDW1-Playback,SDW_JACK_IN_STREAM=SDW1-Capture,\
SDW_SPK_STREAM=SDW2-Playback,SDW_DMIC_STREAM=SDW0-Capture,SDW_AMP_FEEDBACK=false,\
PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-tgl-rt715-rt711-rt1308-mono.bin"

# IPC4 topology for TGL rt711 Headset + rt1308 Amplifier + PCH DMIC
"cavs-sdw\;sof-tgl-rt711-rt1308-4ch\;NUM_SDW_AMPS=1,NUM_DMICS=4,DMIC0_ID=3,\
DMIC1_ID=4,PDM1_MIC_A_ENABLE=1,PDM1_MIC_B_ENABLE=1,SDW_AMP_FEEDBACK=false,\
PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-tgl-rt711-rt1308-4ch.bin"

"cavs-sdw\;sof-adl-rt711-4ch\;DEEPBUFFER_FW_DMA_MS=100,NUM_DMICS=4,DMIC0_ID=2,\
DMIC1_ID=3,PDM1_MIC_A_ENABLE=1,PDM1_MIC_B_ENABLE=1,HDMI1_ID=4,HDMI2_ID=5,\
HDMI3_ID=6,PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-adl-rt711-4ch.bin"

# BT offload
"cavs-nocodec-bt\;sof-nocodec-bt\;PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-nocodec-bt.bin,\
PLATFORM=tgl"
# BT offload loopback test topology (lbm)
"cavs-nocodec-bt\;sof-nocodec-bt-lbm\;BT_LOOPBACK_MODE=true,PLATFORM=tgl,\
PREPROCESS_PLUGINS=nhlt,NHLT_BIN=nhlt-sof-nocodec-bt-lbm.bin"
)
