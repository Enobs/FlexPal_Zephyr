/*
 * xtos-internal.h  --  internal definitions for single-threaded run-time
 *
 * Copyright (c) 2003-2010 Tensilica Inc.
 * Copyright (c) 2019 Intel Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef XTOS_INTERNAL_H
#define XTOS_INTERNAL_H


#if CONFIG_MULTICORE
#include <sof/lib/cpu.h>
#endif
#include <sof/lib/memory.h>
#include <xtensa/config/core.h>
#include <xtensa/xtruntime.h>
#include <xtensa/xtruntime-frames.h>
#include <xtensa/xtensa-versions.h>
#ifndef XTOS_PARAMS_H	/* this to allow indirect inclusion of this header from the outside */
#include "xtos-params.h"
#endif

/*  Relative ordering of subpriorities within an interrupt level (or vector):  */
#define XTOS_SPO_ZERO_LO	0	/* lower (eg. zero) numbered interrupts are lower  priority than higher numbered interrupts */
#define XTOS_SPO_ZERO_HI	1	/* lower (eg. zero) numbered interrupts are higher priority than higher numbered interrupts */


/*  Sanity check some parameters from xtos-params.h:  */
#if XTOS_LOCKLEVEL < XCHAL_EXCM_LEVEL || XTOS_LOCKLEVEL > 15
# error Invalid XTOS_LOCKLEVEL value, must be >= EXCM_LEVEL and <= 15, please fix xtos-params.h
#endif

/*  Mask of interrupts locked out at XTOS_LOCKLEVEL:  */
#define XTOS_LOCKOUT_MASK	XCHAL_INTLEVEL_ANDBELOW_MASK(XTOS_LOCKLEVEL)
/*  Mask of interrupts that can still be enabled at XTOS_LOCKLEVEL:  */
#define XTOS_UNLOCKABLE_MASK	(0xFFFFFFFF-XTOS_LOCKOUT_MASK)

/*  Don't set this:  */
#define XTOS_HIGHINT_TRAMP	0	/* mapping high-pri ints to low-pri not auto-supported */
#define XTOS_VIRTUAL_INTERRUPT	XTOS_HIGHINT_TRAMP	/* partially-virtualized INTERRUPT register not currently supported */
#if XTOS_HIGHINT_TRAMP
# error Automatically-generated high-level interrupt trampolines are not presently supported.
#endif

/*
 *  If single interrupt at level-one, sub-prioritization is irrelevant:
 */
#if defined(XCHAL_INTLEVEL1_NUM)
# undef XTOS_SUBPRI
# define XTOS_SUBPRI 0			/* override - only one interrupt */
#endif

/*
 *  In XEA1, the INTENABLE special register must be virtualized to provide
 *  standard XTOS functionality.
 *  In XEA2, this is only needed for software interrupt prioritization.
 */
#if XTOS_SUBPRI || XCHAL_HAVE_XEA1
#define XTOS_VIRTUAL_INTENABLE	1
#else
#define XTOS_VIRTUAL_INTENABLE	0
#endif

/*
 *  If single interrupt per priority, then fairness is irrelevant:
 */
#if (XTOS_SUBPRI && !XTOS_SUBPRI_GROUPS) || defined(XCHAL_INTLEVEL1_NUM)
# undef XTOS_INT_FAIRNESS
# define XTOS_INT_FAIRNESS	0
#endif

/*  Identify special case interrupt handling code in int-lowpri-dispatcher.S:  */
#define XTOS_INT_SPECIALCASE	(XTOS_SUBPRI_ORDER == XTOS_SPO_ZERO_HI && XTOS_INT_FAIRNESS == 0 && XTOS_SUBPRI_GROUPS == 0)

/*
 *  Determine whether to extend the interrupt entry array:
 */
#define XIE_EXTEND		(XTOS_VIRTUAL_INTENABLE && !XTOS_INT_SPECIALCASE)

/*  If we have the NSAU instruction, ordering of interrupts is reversed in xtos_interrupt_table[]:  */
#if XCHAL_HAVE_NSA
# define MAPINT(n)	((XCHAL_NUM_INTERRUPTS-1)-(n))
# ifdef _ASMLANGUAGE
	.macro	mapint an
	neg 	\an, \an
	addi	\an, \an, XCHAL_NUM_INTERRUPTS-1
	.endm
# endif
#else /* no NSA */
# define MAPINT(n)	(n)
# ifdef _ASMLANGUAGE
	.macro	mapint an
	.endm
# endif
#endif

#define XTOS_TASK_CONTEXT_OFFSET	48

#if defined(_ASMLANGUAGE) || defined(__ASSEMBLER__)
/***********   Useful macros   ***********/

/*
 *  A useful looping macro:
 *  'iterate' invokes 'what' (an instruction, pseudo-op or other macro)
 *  multiple times, passing it a numbered parameter from 'from' to 'to'
 *  inclusively.  Does not invoke 'what' at all if from > to.
 *  Maximum difference between 'from' and 'to' is 99 minus nesting depth
 *  (GNU 'as' doesn't allow nesting deeper than 100).
 */
	.macro	iterate		from, to, what
	.ifeq	((\to-\from) & ~0xFFF)
	\what	\from
	iterate	"(\from+1)", \to, \what
	.endif
	.endm	// iterate



	//  rsilft
	//
	//  Execute RSIL \ar, \tolevel if \tolevel is different than \fromlevel.
	//  This way the RSIL is avoided if we know at assembly time that
	//  it will not change the level.  Typically, this means the \ar register
	//  is ignored, ie. RSIL is used only to change PS.INTLEVEL.
	//
	.macro	rsilft	ar, fromlevel, tolevel
#if XCHAL_HAVE_INTERRUPTS
	.if \fromlevel - \tolevel
	rsil	\ar, \tolevel
	.endif
#endif
	.endm


	//  Save LOOP and MAC16 registers, if configured, to the exception stack
	//  frame pointed to by address register \esf, using \aa and \ab as temporaries.
	//
	//  This macro essentially saves optional registers that the compiler uses by
	//  default when present.
	//  Note that the acclo/acchi subset of MAC16 may be used even if others
	//  multipliers are present (e.g. mul16, mul32).
	//
	//  Only two temp registers required for this code to be optimal (no interlocks) in both
	//  T10xx (Athens) and Xtensa LX microarchitectures (both 5 and 7 stage pipes):
	//
	.macro	save_loops_mac16	esf, aa, ab
#if XCHAL_HAVE_LOOPS
	rsr.lcount	\aa
	rsr.lbeg	\ab
	s32i	\aa, \esf, UEXC_lcount
	rsr.lend	\aa
	s32i	\ab, \esf, UEXC_lbeg
	s32i	\aa, \esf, UEXC_lend
#endif
#if XCHAL_HAVE_MAC16
	rsr.acclo	\aa
	rsr.acchi	\ab
	s32i	\aa, \esf, UEXC_acclo
	s32i	\ab, \esf, UEXC_acchi
# if XTOS_SAVE_ALL_MAC16
	rsr.m0	\aa
	rsr.m1	\ab
	s32i	\aa, \esf, UEXC_mr + 0
	s32i	\ab, \esf, UEXC_mr + 4
	rsr.m2	\aa
	rsr.m3	\ab
	s32i	\aa, \esf, UEXC_mr + 8
	s32i	\ab, \esf, UEXC_mr + 12
# endif
#endif
	.endm

	//  Restore LOOP and MAC16 registers, if configured, from the exception stack
	//  frame pointed to by address register \esf, using \aa, \ab and \ac as temporaries.
	//
	//  Three temp registers are required for this code to be optimal (no interlocks) in
	//  Xtensa LX microarchitectures with 7-stage pipe; otherwise only two
	//  registers would be needed.
	//
	.macro	restore_loops_mac16	esf, aa, ab, ac
#if XCHAL_HAVE_LOOPS
	l32i	\aa, \esf, UEXC_lcount
	l32i	\ab, \esf, UEXC_lbeg
	l32i	\ac, \esf, UEXC_lend
	wsr.lcount	\aa
	wsr.lbeg	\ab
	wsr.lend	\ac
#endif
#if XCHAL_HAVE_MAC16
	l32i	\aa, \esf, UEXC_acclo
	l32i	\ab, \esf, UEXC_acchi
# if XTOS_SAVE_ALL_MAC16
	l32i	\ac, \esf, UEXC_mr + 0
	wsr.acclo	\aa
	wsr.acchi	\ab
	wsr.m0	\ac
	l32i	\aa, \esf, UEXC_mr + 4
	l32i	\ab, \esf, UEXC_mr + 8
	l32i	\ac, \esf, UEXC_mr + 12
	wsr.m1	\aa
	wsr.m2	\ab
	wsr.m3	\ac
# else
	wsr.acclo	\aa
	wsr.acchi	\ab
# endif
#endif
	.endm


/*  Offsets from _xtos_intstruct structure:  */
	.struct 0
#if XTOS_VIRTUAL_INTENABLE
XTOS_ENABLED_OFS:	.space	4	/* _xtos_enabled variable */
XTOS_VPRI_ENABLED_OFS:	.space	4	/* _xtos_vpri_enabled variable */
#endif
#if XTOS_VIRTUAL_INTERRUPT
XTOS_PENDING_OFS:	.space	4	/* _xtos_pending variable */
#endif
	.text


#if XTOS_VIRTUAL_INTENABLE
	// Update INTENABLE register, computing it as follows:
	//	INTENABLE = _xtos_enabled & _xtos_vpri_enabled
	// 			[ & ~_xtos_pending ]
	//
	// Entry:
	//	register ax = &_xtos_intstruct
	//	register ay, az undefined (temporaries)
	//	PS.INTLEVEL set to XTOS_LOCKLEVEL or higher (eg. via xtos_lock)
	//	window overflows prevented (PS.WOE=0, PS.EXCM=1, or overflows
	//		already done for registers ax, ay, az)
	//
	// Exit:
	//	registers ax, ay, az clobbered
	//	PS unchanged
	//	caller needs to SYNC (?) for INTENABLE changes to take effect
	//
	// Note: in other software prioritization schemes/implementations,
	// the term <_xtos_vpri_enabled> in the above expression is often
	// replaced with another expression that computes the set of
	// interrupts allowed to be enabled at the current software virtualized
	// interrupt priority.
	//
	// For example, a simple alternative implementation of software
	// prioritization for XTOS might have been the following:
	//	INTENABLE = _xtos_enabled & (vpri_enabled | UNLOCKABLE_MASK)
	// which removes the need for the interrupt dispatcher to 'or' the
	// UNLOCKABLE_MASK bits into _xtos_vpri_enabled, and lets other code
	// disable all lockout level interrupts by just clearing _xtos_vpri_enabled
	// rather than setting it to UNLOCKABLE_MASK.
	// Other implementations sometimes use a table, eg:
	//	INTENABLE = _xtos_enabled & enable_table[current_vpri]
	// The HAL (used by some 3rd party OSes) uses essentially a table-driven
	// version, with other tables enabling run-time changing of priorities.
	//
	.macro	xtos_update_intenable	ax, ay, az
	//movi	\ax, _xtos_intstruct
	l32i	\ay, \ax, XTOS_VPRI_ENABLED_OFS		// ay = _xtos_vpri_enabled
	l32i	\az, \ax, XTOS_ENABLED_OFS		// az = _xtos_enabled
	//interlock
	and	\az, \az, \ay		// az = _xtos_enabled & _xtos_vpri_enabled
# if XTOS_VIRTUAL_INTERRUPT
	l32i	\ay, \ax, XTOS_PENDING_OFS		// ay = _xtos_pending
	movi	\ax, -1
	xor	\ay, \ay, \ax		// ay = ~_xtos_pending
	and	\az, \az, \ay		// az &= ~_xtos_pending
# endif
	wsr.intenable	\az
	.endm
#endif /* VIRTUAL_INTENABLE */

	.macro	xtos_lock	ax
	rsil    \ax, XTOS_LOCKLEVEL	// lockout
	.endm

	.macro	xtos_unlock	ax
	wsr.ps	\ax			// unlock
	rsync
	.endm

/*  Offsets to XtosIntHandlerEntry structure fields (see below):  */
# define XIE_HANDLER	0
# define XIE_ARG	4
# define XIE_SIZE	8
# if XIE_EXTEND
#  define XIE_VPRIMASK	(XIE_SIZE*XCHAL_NUM_INTERRUPTS+0)	/* if VIRTUAL_INTENABLE [SUBPRI||XEA1] && !SPECIALCASE */
#  define XIE_LEVELMASK	(XIE_SIZE*XCHAL_NUM_INTERRUPTS+4)	/* [fairness preloop]  if FAIRNESS && SUBPRI [&& SUBPRI_GROUPS] */
# endif

/*  To simplify code:  */
# if XCHAL_HAVE_NSA
#  define IFNSA(a,b)	a
# else
#  define IFNSA(a,b)	b
# endif

	// get_prid ax
	// Extracts core id.
	.macro	get_prid ax
#if XCHAL_HAVE_PRID
	rsr.prid	\ax
	extui		\ax, \ax, 0, 8
#else
	movi		\ax, PLATFORM_PRIMARY_CORE_ID
#endif
	.endm

#if CONFIG_MULTICORE
	// xtos_stack_addr_percore ax, ay, stack_primary, stack_secondary, stack_size
	// Retrieves address of end of stack buffer for certain core to register ax.
	.macro	xtos_stack_addr_percore ax, ay, stack_primary_addr, mem_blk_secondary_addr, stack_size
	get_prid	\ax
	bnei		\ax, PLATFORM_PRIMARY_CORE_ID, core_s
	movi		\ax, \stack_primary_addr
	j		exit
core_s:
	addi		\ax, \ax, -1
	movi		\ay, _core_s_size
	mull		\ax, \ax, \ay
	movi		\ay, (HEAP_SYSTEM_S_SIZE + HEAP_SYS_RUNTIME_S_SIZE)
	add		\ax, \ax, \ay
	movi		\ay, \mem_blk_secondary_addr
	add		\ax, \ax, \ay
	j		exit
exit:
	movi		\ay, \stack_size
	add		\ax, \ax, \ay
	.endm

	// xtos_stack_addr_percore_add ax, stack_name, offset
	// Pointer to dedicated interrupt stack + offset.
	.macro	xtos_stack_addr_percore_add ax, stack_name, offset
	get_prid	\ax
	beqz		\ax, core_0
	beqi		\ax, 1, core_1
	beqi		\ax, 2, core_2
	beqi		\ax, 3, core_3
	j		exit
core_0:
	movi		\ax, \stack_name\()0 + (\offset)
	j		exit
core_1:
	movi		\ax, \stack_name\()1 + (\offset)
	j		exit
core_2:
	movi		\ax, \stack_name\()2 + (\offset)
	j		exit
core_3:
	movi		\ax, \stack_name\()3 + (\offset)
	j		exit
exit:
	.endm

	// xtos_addr_percore_add ax, symbol, offset
	// Pointer to structure per core + offset.
	.macro	xtos_addr_percore_add ax, symbol, offset
	xtos_addr_percore	\ax, \symbol
	addi			\ax, \ax, \offset
	.endm

	// xtos_addr_percore_sub ax, symbol, offset
	// Pointer to structure per core - offset.
	.macro	xtos_addr_percore_sub ax, symbol, offset
	xtos_addr_percore	\ax, \symbol
	addi			\ax, \ax, -\offset
	.endm
#endif /* CONFIG_MULTICORE */

	// xtos_addr_percore ax, structure_name
	// Pointer to structure per core.
	.macro	xtos_addr_percore ax, structure_name
#if XCHAL_HAVE_THREADPTR
	rur.threadptr	\ax
#else
	j 1f
	.align 4
	.literal_position
2:
	.word SOF_VIRTUAL_THREAD_BASE
1:
	.align 4
	l32r	\ax, 2b
	l32i		\ax, \ax, 0
#endif
	l32i		\ax, \ax, XTOS_PTR_TO_\structure_name
	.endm

	// xtos_store_percore ax, ay, structure_name
	// Stores register value under the selected structure per core.
	.macro	xtos_store_percore ax, ay, structure_name
#if XCHAL_HAVE_THREADPTR
	rur.threadptr	\ay
#else
	j 1f
	.align 4
	.literal_position
2:
	.word SOF_VIRTUAL_THREAD_BASE
1:
	.align 4
	l32r	\ay, 2b
	l32i	\ay, \ay, 0
#endif
	s32i		\ax, \ay, XTOS_PTR_TO_\structure_name
	.endm

	// xtos_int_stack_addr_percore ax, int_level, stack_name
	// Pointer to dedicated interrupt stack.
	.macro	xtos_int_stack_addr_percore ax, int_level, stack_name
#if XCHAL_HAVE_THREADPTR
	rur.threadptr	\ax
#else
	j 1f
	.align 4
	.literal_position
2:
	.word SOF_VIRTUAL_THREAD_BASE
1:
	.align 4
	l32r	\ax, 2b
	l32i		\ax, \ax, 0
#endif
	l32i		\ax, \ax, XTOS_PTR_TO_\stack_name\()_&int_level
	.endm

	// xtos_task_ctx_percore ax
	// Pointer to structure per core.
	.macro	xtos_task_ctx_percore ax
#if XCHAL_HAVE_THREADPTR
	rur.threadptr	\ax
#else
	j 1f
	.align 4
	.literal_position
2:
	.word SOF_VIRTUAL_THREAD_BASE
1:
	.align 4
	l32r	\ax, 2b
	l32i		\ax, \ax, 0
#endif
	l32i		\ax, \ax, XTOS_TASK_CONTEXT_OFFSET
	.endm

	// xtos_task_ctx_store_percore ax, ay
	// Changes task context to point to the selected address.
	.macro	xtos_task_ctx_store_percore ax, ay
#if XCHAL_HAVE_THREADPTR
	rur.threadptr	\ay
#else
	j 1f
	.align 4
	.literal_position
2:
	.word SOF_VIRTUAL_THREAD_BASE
1:
	.align 4
	l32r	\ay, 2b
	l32i	\ay, \ay, 0
#endif
	s32i		\ax, \ay, XTOS_TASK_CONTEXT_OFFSET
	.endm

	// Executes optional callback on wake up
	.macro	xtos_on_wakeup
#if CONFIG_WAKEUP_HOOK
	call12 arch_interrupt_on_wakeup
#endif
	.endm

#else /* !_ASMLANGUAGE && !__ASSEMBLER__ */

/*
 *  Interrupt handler table entry.
 *  Unregistered entries have 'handler' point to xtos_unhandled_interrupt().
 */
typedef struct XtosIntHandlerEntry {
    _xtos_handler	handler;
    union {
        void *		varg;
        int		narg;
    } u;
} XtosIntHandlerEntry;
# if XIE_EXTEND
typedef struct XtosIntMaskEntry {
    unsigned		vpri_mask;	/* mask of interrupts enabled when this interrupt is taken */
    unsigned		level_mask;	/* mask of interrupts at this interrupt's level */
} XtosIntMaskEntry;
# endif

#if CONFIG_MULTICORE
struct XtosIntStruct
{
	unsigned xtos_enabled;
	unsigned vpri_enabled;
};

// XtosIntInterruptTable holds array of interrupt handler descriptors.
struct XtosIntInterruptTable
{
	struct XtosIntHandlerEntry array[XCHAL_NUM_INTERRUPTS];
};

// XtosInterruptStructure describes layout of xtos interrupt structures per core
// generated for certain platform in file interrupt-table.S.
struct XtosInterruptStructure
{
	struct XtosIntStruct xtos_enabled;
	struct XtosIntInterruptTable xtos_interrupt_table;
	struct XtosIntMaskEntry xtos_interrupt_mask_table[XCHAL_NUM_INTERRUPTS];
	__attribute__((aligned(XCHAL_DCACHE_LINESIZE))) int al[0];
};
#endif

extern void xtos_unhandled_interrupt();

#endif /* !_ASMLANGUAGE && !__ASSEMBLER__ */

/*
 *  Notes...
 *
 *  XEA1 and interrupt-SUBPRIoritization both imply virtualization of INTENABLE.
 *  Synchronous trampoloines imply partial virtualization of the INTERRUPT
 *  register, which in turn also implies virtualization of INTENABLE register.
 *  High-level interrupts manipulating the set of enabled interrupts implies
 *  at least a high XTOS_LOCK_LEVEL, although not necessarily INTENABLE virtualization.
 *
 *  With INTENABLE register virtualization, at all times the INTENABLE
 *  register reflects the expression:
 *	(set of interrupts enabled) & (set of interrupts enabled by current
 *					virtual priority)
 *
 *  Unrelated (DBREAK semantics):
 *
 *	A[31-6] = DBA[3-6]
 *	---------------------
 *	A[5-0] & DBC[5-C] & szmask
 *
 *	= DBA[5-0] & szmask
 *			^___  ???
 */


/*  Report whether the XSR instruction is available (conservative):  */
#define HAVE_XSR	(XCHAL_HAVE_XEA2 || !XCHAL_HAVE_EXCEPTIONS)
/*
 *  This is more accurate, but not a reliable test in software releases prior to 6.0
 *  (where the targeted hardware parameter was not explicit in the XPG):
 *
 *#define HAVE_XSR	(XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_T1040_0)
 */



/* Macros for supporting hi-level and medium-level interrupt handling. */

#if XCHAL_NUM_INTLEVELS > 6
#error Template files (*-template.S) limit support to interrupt levels <= 6
#endif

#if  defined(__XTENSA_WINDOWED_ABI__) && XCHAL_HAVE_CALL4AND12 == 0
#error CALL8-only is not supported!
#endif

#define INTERRUPT_IS_HI(level)  \
	( XCHAL_HAVE_INTERRUPTS && \
	 (XCHAL_EXCM_LEVEL < level) && \
	 (XCHAL_NUM_INTLEVELS >= level) && \
	 (XCHAL_HAVE_DEBUG ? XCHAL_DEBUGLEVEL != level : 1))

#define INTERRUPT_IS_MED(level) \
	(XCHAL_HAVE_INTERRUPTS && (XCHAL_EXCM_LEVEL >= level))


#define _JOIN(x,y)	x ## y
#define JOIN(x,y)	_JOIN(x,y)

#define _JOIN3(a,b,c)	a ## b ## c
#define JOIN3(a,b,c)	_JOIN3(a,b,c)

#define LABEL(x,y)		JOIN3(x,_INTERRUPT_LEVEL,y)
#define EXCSAVE_LEVEL		JOIN(EXCSAVE_,_INTERRUPT_LEVEL)
#define INTLEVEL_VSIZE		JOIN3(XSHAL_INTLEVEL,_INTERRUPT_LEVEL,_VECTOR_SIZE)

/*  For asm macros; works for positive a,b smaller than 1000:  */
#define GREATERTHAN(a, b)	(((b) - (a)) & ~0xFFF)
#define EQUAL(a, b)		((1 << (a)) & (1 << (b)))

#if CONFIG_MULTICORE
// sizeof(xtos_enabled)
#define XTOS_ENABLED_SIZE_PER_CORE	(4)
// sizeof(vpri_enabled)
#define XTOS_VPRI_ENABLED_SIZE_PER_CORE	(4)
// sizeof(XtosIntStruct)
#define XTOS_INTSTRUCT_SIZE_PER_CORE	(XTOS_ENABLED_SIZE_PER_CORE + \
					XTOS_VPRI_ENABLED_SIZE_PER_CORE)
#endif

#endif /* XTOS_INTERNAL_H */

