// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Amery Song <chao.song@intel.com>
//	   Keyon Jie <yang.jie@linux.intel.com>

#include <sof/audio/format.h>
#include <sof/common.h>
#include <rtos/alloc.h>
#include <sof/math/fft.h>

#ifdef FFT_GENERIC
#include <sof/audio/coefficients/fft/twiddle_32.h>

/*
 * These helpers are optimized for FFT calculation only.
 * e.g. _add/sub() assume the output won't be saturate so no check needed,
 * and _mul() assumes Q1.31 * Q1.31 so the output will be shifted to be Q1.31.
 */

static inline void icomplex32_add(const struct icomplex32 *in1, const struct icomplex32 *in2,
				  struct icomplex32 *out)
{
	out->real = in1->real + in2->real;
	out->imag = in1->imag + in2->imag;
}

static inline void icomplex32_sub(const struct icomplex32 *in1, const struct icomplex32 *in2,
				  struct icomplex32 *out)
{
	out->real = in1->real - in2->real;
	out->imag = in1->imag - in2->imag;
}

static inline void icomplex32_mul(const struct icomplex32 *in1, const struct icomplex32 *in2,
				  struct icomplex32 *out)
{
	out->real = ((int64_t)in1->real * in2->real - (int64_t)in1->imag * in2->imag) >> 31;
	out->imag = ((int64_t)in1->real * in2->imag + (int64_t)in1->imag * in2->real) >> 31;
}

/* complex conjugate */
static inline void icomplex32_conj(struct icomplex32 *comp)
{
	comp->imag = SATP_INT32((int64_t)-1 * comp->imag);
}

/* shift a complex n bits, n > 0: left shift, n < 0: right shift */
static inline void icomplex32_shift(const struct icomplex32 *input, int32_t n,
				    struct icomplex32 *output)
{
	if (n > 0) {
		/* need saturation handling */
		output->real = SATP_INT32(SATM_INT32((int64_t)input->real << n));
		output->imag = SATP_INT32(SATM_INT32((int64_t)input->imag << n));
	} else {
		output->real = input->real >> -n;
		output->imag = input->imag >> -n;
	}
}

/**
 * \brief Execute the 32-bits Fast Fourier Transform (FFT) or Inverse FFT (IFFT)
 *	  For the configured fft_pan.
 * \param[in] plan - pointer to fft_plan which will be executed.
 * \param[in] ifft - set to 1 for IFFT and 0 for FFT.
 */
void fft_execute_32(struct fft_plan *plan, bool ifft)
{
	struct icomplex32 tmp1;
	struct icomplex32 tmp2;
	struct icomplex32 *inb;
	struct icomplex32 *outb;
	int depth;
	int top;
	int bottom;
	int index;
	int i;
	int j;
	int k;
	int m;
	int n;

	if (!plan || !plan->bit_reverse_idx)
		return;

	inb = plan->inb32;
	outb = plan->outb32;
	if (!inb || !outb)
		return;

	/* convert to complex conjugate for ifft */
	if (ifft) {
		for (i = 0; i < plan->size; i++)
			icomplex32_conj(&inb[i]);
	}

	/* step 1: re-arrange input in bit reverse order, and shrink the level to avoid overflow */
	for (i = 1; i < plan->size; ++i)
		icomplex32_shift(&inb[i], -(plan->len), &outb[plan->bit_reverse_idx[i]]);

	/* step 2: loop to do FFT transform in smaller size */
	for (depth = 1; depth <= plan->len; ++depth) {
		m = 1 << depth;
		n = m >> 1;
		i = FFT_SIZE_MAX >> depth;

		/* doing FFT transforms in size m */
		for (k = 0; k < plan->size; k += m) {
			/* doing one FFT transform for size m */
			for (j = 0; j < n; ++j) {
				index = i * j;
				top = k + j;
				bottom = top + n;
				tmp1.real = twiddle_real_32[index];
				tmp1.imag = twiddle_imag_32[index];
				/* calculate the accumulator: twiddle * bottom */
				icomplex32_mul(&tmp1, &outb[bottom], &tmp2);
				tmp1 = outb[top];
				/* calculate the top output: top = top + accumulate */
				icomplex32_add(&tmp1, &tmp2, &outb[top]);
				/* calculate the bottom output: bottom = top - accumulate */
				icomplex32_sub(&tmp1, &tmp2, &outb[bottom]);
			}
		}
	}

	/* shift back for ifft */
	if (ifft) {
		/*
		 * no need to divide N as it is already done in the input side
		 * for Q1.31 format. Instead, we need to multiply N to compensate
		 * the shrink we did in the FFT transform.
		 */
		for (i = 0; i < plan->size; i++)
			icomplex32_shift(&outb[i], plan->len, &outb[i]);
	}
}

#endif
