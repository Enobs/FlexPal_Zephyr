/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Michal Jerzy Wierzbicki <michalx.wierzbicki@intel.com>
 */

#ifndef __SOF_TRACE_PREPROC_H__
#define __SOF_TRACE_PREPROC_H__

/* Macros in this file are to be invoked directly from code.
 * In order to work, they require a number of other macros that are
 * defined in the header file specified below.
 * Macros from the file specified below are not to meant to be used
 * directly / independently.
 * For more detailed commentary of innards of macros in this file,
 * see file specified below.
 */
#include <sof/trace/preproc-private.h>
#include <stdint.h>

/* count number of var args - during preprocesing
 * works for predefined number of args
 * META_COUNT_VARAGS_BEFORE_COMPILE(A,B,C,D) evaluates to 4
 */
#define META_COUNT_VARAGS_BEFORE_COMPILE(...)\
	META_DEC(\
		_META_PP_NARG_BEFORE_COMPILE_(\
			_, ##__VA_ARGS__, _META_PP_RSEQ_N()\
		)\
	)

/* treat x as string while forcing x expansion beforehand */
#define META_QUOTE(x) _META_QUOTE(x)

/* concat x and y while forcing x and y expansion beforehand */
#define META_CONCAT(x, y) _META_CONCAT_BASE(x, y)

/* discard first x-1 args in vararg and return the xth arg */

#define META_GET_ARG_N(n, ...) META_CONCAT(_META_GET_ARG_, n)(__VA_ARGS__)

#define META_HAS_ARGS(...) META_BOOL(\
	_META_GET_ARG_1(_META_NO_ARGS __VA_ARGS__)()\
)

/* Only META_NOT(0)   evaulates to 1
 * notice, that any x!=0 would also result in 0
 * e.x. META_NOT(123) evaluates to 0
 */
#define META_NOT(x) _META_IS_PROBE(META_CONCAT(_META_NOT_, x))
/* hacky way to convert tokens into 0 1*/
#define META_BOOL(x) META_NOT(META_NOT(x))

/* META_IF_ELSE(X)(a)(b) expands to
 * b for X == 0
 * a for X != 0
 */
#define META_IF_ELSE(condition) _META_IF_ELSE(META_BOOL(condition))

/* same story with indirection as META_IF_ELSE */
#define META_IF(condition) _META_IIF(META_BOOL(condition))

/* primitive recursion
 * default depth is 8
 */
#define META_RECURSE(...) _META_REQRS_8(__VA_ARGS__)
/* choose explicitly depth of recursion
 */
#define META_RECURSE_N(depth, ...)\
	META_CONCAT(_META_REQRS_, depth)(__VA_ARGS__)

/* The only sane way I found to increment values in cpreproc */
#define META_INC(x) META_CONCAT(_META_INC_, x)

/* The only sane way I found to decrement values in cpreproc */
#define META_DEC(x) META_CONCAT(_META_DEC_, x)

/* Delay macro m expansion depth times
 * by writing META_DEFER(0, m)(args) we expand it in 1st scan
 * by writing META_DEFER(1, m)(args) we expand it in 2nd scan
 * ...
 * by writing META_DEFER(n, m)(args) we expand it in n+1nth scan
 */
#define META_DEFER(depth, m) m _META_DEFER_N(depth)

/* while(count--!=0) do
 * uses DEC so count == N can only work if all following exist
 * DEC_0, DEC_1, ..., DEC_N-1, DEC_N
 */
#define META_REPEAT(count, macro, ...)\
	_META_WHEN(count)\
	(\
		_META_DEFER_2(_META_REPEAT_INDIRECT) ()  \
			(META_DEC(count), macro, __VA_ARGS__)\
		_META_DEFER_2(macro)\
			(META_DEC(count),        __VA_ARGS__)\
	)

/* map every group of arg_count arguments onto function m
 * i.e. arg_count=2;m=ADD;args=1,2,3,4,5,6,7...
 * results in ADD(1,2) ADD(3,4) ADD(5,6) and so on
 * MAP##N must exist for arg_count == N to work
 */
#define META_MAP(arg_count, m, ...) META_RECURSE(\
	_META_MAP_BODY(arg_count, m, __VA_ARGS__))

/* map aggregator and every group of arg_count arguments onto function m
 * i.e. aggr=x;arg_count=1;m=ADD;args=1,2,3,4,5,6,7...
 * results in x = ... ADD(7,ADD(6,ADD(5,ADD(4,ADD(3,ADD(2,ADD(1,x))))))) ...
 * MAP##N must exist for arg_count == N to work
 */
#define META_MAP_AGGREGATE(arg_count, m, aggr, ...)\
	META_CONCAT(_META_MAP_AGGREGATE_, arg_count)(m, aggr, __VA_ARGS__)

/* META_CONCAT_SEQ is basicaly variadic version of macro META_CONCAT
 * META_CONCAT_SEQ(A,B,C,D) tokenizes to ABCD
 */
#define META_CONCAT_SEQ(aggr, ...) META_RECURSE(\
	META_MAP_AGGREGATE(1, META_CONCAT, aggr, __VA_ARGS__))

/* META_CONCAT with parametrised delimeter between concatenised tokens
 * META_CONCAT_SEQ_DELIM_(A,B,C,D) tokenizes as A_B_C_D
 */
#define META_CONCAT_SEQ_DELIM_(aggr, ...) META_RECURSE(\
	META_MAP_AGGREGATE(1, _META_CONCAT_DELIM_, aggr, __VA_ARGS__))

/* META_SEQ_FROM_0_TO(3, META_SEQ_STEP_param)
 * produces , param0 , param1 , param2
 */
#define META_SEQ_FROM_0_TO(arg_count, func)\
	META_RECURSE(META_REPEAT(arg_count, func, ~))

/* Macros to be used as 2nd argument of macro META_SEQ_FROM_0_TO
 * for instance
 * META_SEQ_FROM_0_TO(arg_count, META_SEQ_STEP)
 * produces
 * 0 1 2 3 4
 */
#define META_SEQ_STEP(i, _) i
#define META_SEQ_STEP_param(i, _) , META_CONCAT(param, i)
#define META_SEQ_STEP_param_uint32_t(i, _) , uint32_t META_CONCAT(param, i)
#define META_SEQ_STEP_param_uint64_t(i, _) , uint64_t META_CONCAT(param, i)
#define META_SEQ_STEP_param_int32_t( i, _) ,  int32_t META_CONCAT(param, i)
#define META_SEQ_STEP_param_int64_t( i, _) ,  int64_t META_CONCAT(param, i)

#define META_SEQ_STEP_id(i, _) , META_CONCAT(id_, i)
#define META_SEQ_STEP_id_uint32_t(i, _) , uint32_t META_CONCAT(id_, i)

/* generates function signature
 * for instance with:
 *   prefix=foo ; postfix=__bar ; return_t=void
 *   args=(int x, int y)
 * will produce:
 *  void foo_bar(int x, int y)
 */
#define META_FUNC_WITH_VARARGS(prefix, postfix, return_t, args)\
		return_t META_CONCAT(prefix, postfix) (args)

/* counteract compiler warning about unused variables */
#define UNUSED(arg1, ...) do { META_RECURSE( \
	META_MAP_AGGREGATE(1, _META_VOID2, _META_VOID(arg1), __VA_ARGS__)); \
	} while (0)

#endif /* __SOF_TRACE_PREPROC_H__ */
