// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2022 Intel Corporation. All rights reserved.
 *
 * Author: Shriram Shastry <malladi.sastry@linux.intel.com>
 *
 */

/* Include Files */
#include <sof/math/exp_fcn.h>
#include <sof/math/numbers.h>
#include <sof/common.h>
#include <rtos/bit.h>
#include <stdbool.h>
#include <stdint.h>

#define BIT_MASK_Q62P2 0x4000000000000000LL
#define CONVERG_ERROR 28823037607936LL	// error smaller than 1e-4,1/2 ^ -44.7122876200884
#define BIT_MASK_LOW_Q27P5 0x8000000
#define QUOTIENT_SCALE BIT(30)
#define TERMS_Q23P9 8388608
#define LSHIFT_BITS 8192

/* inv multiplication lookup table */
/* LUT = ceil(1/factorial(b_n) * 2 ^ 63) */
static const int64_t exp_iv_ilookup[19] = {
	4611686018427387904LL,
	1537228672809129301LL,
	384307168202282325LL,
	76861433640456465LL,
	12810238940076077LL,
	1830034134296582LL,
	228754266787072LL,
	25417140754119LL,
	2541714075411LL,
	231064915946LL,
	19255409662LL,
	1481185358LL,
	105798954LL,
	7053264LL,
	440829LL,
	25931LL,
	1441LL,
	76LL,
	4LL
};

/* Function Declarations */
/*
 * Arguments	: int64_t in_0
 *		  int64_t in_1
 *		  uint64_t *ptroutbitshi
 *		  uint64_t *ptroutbitslo
 * Multiplication of two signed int64 numbers
 * Return Type	: void
 * Description:Perform element-wise multiplication on in_0 and in_1
 * while keeping the required product word length and fractional
 * length in mind. mul_s64 function divide the 64-bit quantities
 * into two 32-bit words, multiply the low words to produce the
 * lowest and second-lowest words in the result, then both pairs
 * of low and high words from different numbers to produce the
 * second and third lowest words in the result, and finally both
 * high words to produce the two highest words in the outcome.
 * Add them all up, taking carry into consideration.
 *
 * The 64 x 64 bit multiplication of operands in_0 and in_1 is
 * shown in the image below. The 64-bit operand in_0, in_1 is
 * represented by the notation in0_H, in1_H for the top 32 bits
 * and in0_L, in1_L for the bottom 32 bits.
 *
 *				in0_H : in0_L
 *			x	in1_H : in1_L
 *			---------------------
 *	P0			in0_L x in1_L
 *	P1		in0_H x in1_L		64 bit inner multiplication
 *	P2		in0_L x in1_H		64 bit inner multiplication
 *	P3	in0_H x in1_H
 *			--------------------
 *			[64 x 64 bit multiplication] sum of inner products
 * All combinations are multiplied by one another and then added.
 * Each inner product is moved into its proper power location, given the names
 * of the inner products, redoing the addition where 000 represents 32 zero
 * bits. The inner products can be added together in 64 bit addition. The sum
 * of two 64-bit numbers yields a 65-bit output.
 *		   (P0H:P0L)
 *		P1H(P1L:000)
 *		P2H(P2L:000)
 *	P3H:P3L(000:000)
 *	.......(aaa:P0L)
 * By combining P0H:P0L and P1L:000. This can lead to a carry, denote as CRY0.
 * The partial result is then multiplied by P2L:000.
 * We call it CRY1 because it has the potential to carry again.
 *	(CRY0 + CRY1)P0H:P0L
 *	(	 P1H)P1L:000
 *	(	 P2H)P2L:000
 *	(P3H:	 P3L)000:000
 *	--------------------
 *	(ccc:bbb)aaa:P0L
 * P1H, P2H, and P3H:P3L are added to the carry CRY0 + CRY1. This increase will
 * not result in an overflow.
 *
 */
static inline void mul_s64(int64_t in_0, int64_t in_1, uint64_t *ptroutbitshi,
			   uint64_t *ptroutbitslo)
{
	uint64_t absin0 = ABS(in_0);
	uint64_t absin1 = ABS(in_1);
	uint64_t in0hi = absin0 >> 32;
	uint64_t in0lo = absin0 & UINT32_MAX;
	uint64_t in1hi = absin1 >> 32;
	uint64_t prodhilo;
	uint64_t prodlohi;

	absin0 = absin1 & UINT32_MAX;
	/* multiplication */
	prodhilo = in0hi * absin0;
	prodlohi = in0lo * in1hi;
	absin0 *= in0lo;

	in0lo = absin0 + (prodlohi << 32);
	absin1 = in0lo < absin0 ? 1 : 0;
	absin0 = in0lo;
	/* shift and add */
	in0lo += prodhilo << 32;
	if (in0lo < absin0)
		absin1++;
	/* carry */
	absin0 = absin1 + in0hi * in1hi + (prodlohi >> 32) + (prodhilo >> 32);
	/* 2's complement */
	if (in_0  && in_1  && (in_0 > 0) != (in_1 > 0)) {
		/* bit inversion */
		absin0 = ~absin0;
		in0lo = ~in0lo;
		/* add to low byte */
		in0lo++;
		if (!in0lo)
			absin0++;
	}
	/* pointer- output high and low bytes */
	*ptroutbitshi = absin0;
	*ptroutbitslo = in0lo;
}

/*
 * Arguments	: int64_t a
 *		  int64_t b
 * Return Type	: int64_t
 */
static inline int64_t lomul_s64_sr_sat_near(int64_t a, int64_t b)
{
	uint64_t u64_rhi;
	uint64_t u64_rlo;

	mul_s64(a, b, &u64_rhi, &u64_rlo);
	const bool roundup = (u64_rlo & BIT_MASK_LOW_Q27P5) != 0;

	u64_rlo = (u64_rhi << 36 | u64_rlo >> 28) + (roundup ? 1 : 0);
	return u64_rlo;
}

/* function f(x) = a^x, x is variable and a is base
 *
 * Arguments	: int32_t x(Q4.28)
 * input range -5 to 5
 *
 * Return Type	: int32_t ts(Q9.23)
 * output range 0.0067465305 to 148.4131488800
 *+------------------+-----------------+--------+--------+
 *| x		     | ts (returntype) |   x    |  ts    |
 *+----+-----+-------+----+----+-------+--------+--------+
 *|WLen| FLen|Signbit|WLen|FLen|Signbit| Qformat| Qformat|
 *+----+-----+-------+----+----+-------+--------+--------+
 *| 32 | 28  |  1    | 32 | 23 |   0   | 4.28   | 9.23   |
 *+------------------+-----------------+--------+--------+
 */
int32_t sofm_exp_int32(int32_t x)
{
	uint64_t ou0Hi;
	uint64_t ou0Lo;
	int64_t qt;
	int32_t b_n;
	int32_t ts =  TERMS_Q23P9; /* Q23.9 */
	int64_t dividend = (x + LSHIFT_BITS) >> 14; /* x in Q50.14 */
	static const int32_t i_emin = -1342177280; /* Q4.28 */
	static const int32_t o_emin = 56601; /* Q9.23 */
	static const int32_t i_emax = 1342177280; /* Q4.28 */
	static const int32_t o_emax = 1244979733; /* Q9.23 */

	/* Out of range input(x>5, x<-5), */
	/* return clipped value x > 5= e^5, and x<-5 = e^-5 */
	if (x < i_emin)
		return o_emin; /* 0.0067473649978638 in Q9.23 */

	if (x > i_emax)
		return o_emax; /* 148.4131494760513306 in Q9.23 */

	/* pre-computation of 1st & 2nd terms */
	mul_s64(dividend, BIT_MASK_Q62P2, &ou0Hi, &ou0Lo);
	qt = (ou0Hi << 46) | (ou0Lo >> 18);/* Q6.26 */
	ts += (int32_t)((qt >> 35) + ((qt & QUOTIENT_SCALE) >> 18));
	dividend = lomul_s64_sr_sat_near(dividend, x);
	for (b_n = 0; b_n < ARRAY_SIZE(exp_iv_ilookup); b_n++) {
		mul_s64(dividend, exp_iv_ilookup[b_n], &ou0Hi, &ou0Lo);
		qt = (ou0Hi << 45) | (ou0Lo >> 19);

		/* sum of the remaining terms */
		ts += (int32_t)((qt >> 35) + ((qt & QUOTIENT_SCALE) ? 1 : 0));
		dividend = lomul_s64_sr_sat_near(dividend, x);

		qt  = ABS(qt);
		/* For inputs between -5 and 5, (qt < CONVERG_ERROR) is always true */
		if (qt < CONVERG_ERROR)
			break;
	}
	return ts;
}
