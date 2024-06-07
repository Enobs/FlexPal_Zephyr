// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Intel Corporation. All rights reserved.
//
// Author: Amery Song <chao.song@intel.com>
//	   Keyon Jie <yang.jie@linux.intel.com>

#include <sof/audio/format.h>
#include <sof/common.h>
#include <sof/math/fft.h>

#ifdef FFT_GENERIC
#include <sof/audio/coefficients/fft/twiddle_16.h>

/*
 * Helpers for 16 bit FFT calculation
 */
static inline void icomplex16_add(const struct icomplex16 *in1, const struct icomplex16 *in2,
				  struct icomplex16 *out)
{
	out->real = in1->real + in2->real;
	out->imag = in1->imag + in2->imag;
}

static inline void icomplex16_sub(const struct icomplex16 *in1, const struct icomplex16 *in2,
				  struct icomplex16 *out)
{
	out->real = in1->real - in2->real;
	out->imag = in1->imag - in2->imag;
}

static inline void icomplex16_mul(const struct icomplex16 *in1, const struct icomplex16 *in2,
				  struct icomplex16 *out)
{
	int32_t real = (int32_t)in1->real * in2->real - (int32_t)in1->imag * in2->imag;
	int32_t imag = (int32_t)in1->real * in2->imag + (int32_t)in1->imag * in2->real;

	out->real = Q_SHIFT_RND(real, 30, 15);
	out->imag = Q_SHIFT_RND(imag, 30, 15);
}

/* complex conjugate */
static inline void icomplex16_conj(struct icomplex16 *comp)
{
	comp->imag = sat_int16(-((int32_t)comp->imag));
}

/* shift a complex n bits, n > 0: left shift, n < 0: right shift */
static inline void icomplex16_shift(const struct icomplex16 *input, int16_t n,
				    struct icomplex16 *output)
{
	int n1, n2;

	if (n >= 0) {
		/* need saturation handling */
		output->real = sat_int16((int32_t)input->real << n);
		output->imag = sat_int16((int32_t)input->imag << n);
	} else {
		n1 = -n;
		n2 = 1 << (n1 - 1);
		output->real = sat_int16(((int32_t)input->real + n2) >> n1);
		output->imag = sat_int16(((int32_t)input->imag  + n2) >> n1);
	}
}

/**
 * \brief Execute the 16-bits Fast Fourier Transform (FFT) or Inverse FFT (IFFT)
 *	  For the configured fft_pan.
 * \param[in] plan - pointer to fft_plan which will be executed.
 * \param[in] ifft - set to 1 for IFFT and 0 for FFT.
 */
void fft_execute_16(struct fft_plan *plan, bool ifft)
{
	struct icomplex16 tmp1;
	struct icomplex16 tmp2;
	struct icomplex16 *inb;
	struct icomplex16 *outb;
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

	inb = plan->inb16;
	outb = plan->outb16;
	if (!inb || !outb)
		return;

	/* convert to complex conjugate for ifft */
	if (ifft) {
		for (i = 0; i < plan->size; i++)
			icomplex16_conj(&inb[i]);
	}

	/* step 1: re-arrange input in bit reverse order, and shrink the level to avoid overflow */
	for (i = 1; i < plan->size; ++i)
		icomplex16_shift(&inb[i], -(plan->len), &outb[plan->bit_reverse_idx[i]]);

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
				tmp1.real = twiddle_real_16[index];
				tmp1.imag = twiddle_imag_16[index];
				/* calculate the accumulator: twiddle * bottom */
				icomplex16_mul(&tmp1, &outb[bottom], &tmp2);
				tmp1 = outb[top];
				/* calculate the top output: top = top + accumulate */
				icomplex16_add(&tmp1, &tmp2, &outb[top]);
				/* calculate the bottom output: bottom = top - accumulate */
				icomplex16_sub(&tmp1, &tmp2, &outb[bottom]);
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
			icomplex16_shift(&outb[i], plan->len, &outb[i]);
	}
}
#endif
