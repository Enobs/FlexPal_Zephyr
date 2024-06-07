/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2017 Intel Corporation. All rights reserved.
 *
 * Author: Seppo Ingalsuo <seppo.ingalsuo@linux.intel.com>
 */

#ifndef __SOF_AUDIO_EQ_FIR_EQ_FIR_H__
#define __SOF_AUDIO_EQ_FIR_EQ_FIR_H__

#include <sof/audio/module_adapter/module/generic.h>
#include <sof/audio/audio_stream.h>
#include <sof/audio/format.h>
#include <sof/math/fir_config.h>
#if FIR_GENERIC
#include <sof/math/fir_generic.h>
#endif
#if FIR_HIFIEP
#include <sof/math/fir_hifi2ep.h>
#endif
#if FIR_HIFI3
#include <sof/math/fir_hifi3.h>
#endif
#include <user/fir.h>
#include <stdint.h>

/** \brief Macros to convert without division bytes count to samples count */
#define EQ_FIR_BYTES_TO_S16_SAMPLES(b)	((b) >> 1)
#define EQ_FIR_BYTES_TO_S32_SAMPLES(b)	((b) >> 2)

#if CONFIG_FORMAT_S16LE
void eq_fir_s16(struct fir_state_32x16 *fir, struct input_stream_buffer *bsource,
		struct output_stream_buffer *bsink, int frames);

void eq_fir_2x_s16(struct fir_state_32x16 *fir, struct input_stream_buffer *bsource,
		   struct output_stream_buffer *bsink, int frames);
#endif /* CONFIG_FORMAT_S16LE */

#if CONFIG_FORMAT_S24LE
void eq_fir_s24(struct fir_state_32x16 *fir, struct input_stream_buffer *bsource,
		struct output_stream_buffer *bsink, int frames);

void eq_fir_2x_s24(struct fir_state_32x16 *fir, struct input_stream_buffer *bsource,
		   struct output_stream_buffer *bsink, int frames);
#endif /* CONFIG_FORMAT_S24LE */

#if CONFIG_FORMAT_S32LE
void eq_fir_s32(struct fir_state_32x16 *fir, struct input_stream_buffer *bsource,
		struct output_stream_buffer *bsink, int frames);

void eq_fir_2x_s32(struct fir_state_32x16 *fir, struct input_stream_buffer *bsource,
		   struct output_stream_buffer *bsink, int frames);
#endif /* CONFIG_FORMAT_S32LE */

#ifdef UNIT_TEST
void sys_comp_module_eq_fir_interface_init(void);
#endif

#endif /* __SOF_AUDIO_EQ_FIR_EQ_FIR_H__ */
