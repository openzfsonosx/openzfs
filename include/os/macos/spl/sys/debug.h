/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2010 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Copyright (c) 2012 by Delphix. All rights reserved.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */


/*
 * Available Solaris debug functions.  All of the ASSERT() macros will be
 * compiled out when NDEBUG is defined, this is the default behavior for
 * the SPL.  To enable assertions use the --enable-debug with configure.
 * The VERIFY() functions are never compiled out and cannot be disabled.
 *
 * PANIC()	- Panic the node and print message.
 * ASSERT()	- Assert X is true, if not panic.
 * ASSERTF()	- Assert X is true, if not panic and print message.
 * ASSERTV()	- Wraps a variable declaration which is only used by ASSERT().
 * ASSERT3S()	- Assert signed X OP Y is true, if not panic.
 * ASSERT3U()	- Assert unsigned X OP Y is true, if not panic.
 * ASSERT3P()	- Assert pointer X OP Y is true, if not panic.
 * ASSERT0()	- Assert value is zero, if not panic.
 * VERIFY()	- Verify X is true, if not panic.
 * VERIFY3S()	- Verify signed X OP Y is true, if not panic.
 * VERIFY3U()	- Verify unsigned X OP Y is true, if not panic.
 * VERIFY3P()	- Verify pointer X OP Y is true, if not panic.
 * VERIFY0()	- Verify value is zero, if not panic.
 */

#ifndef _SPL_DEBUG_H
#define _SPL_DEBUG_H


#ifdef __cplusplus
extern "C" {
#endif


// VERIFY macros will always call panic
extern void panic(const char *string, ...);
#define	PANIC panic

#define VERIFY(cond)									\
	do {												\
		if (unlikely(!(cond)))							\
			PANIC("VERIFY(" #cond ") failed\n");		\
	} while (0)

#define VERIFY3_IMPL(LEFT, OP, RIGHT, TYPE, FMT, CAST)					\
	do {																\
		TYPE _verify3_left = (TYPE)(LEFT);								\
		TYPE _verify3_right = (TYPE)(RIGHT);							\
		if (!(_verify3_left OP _verify3_right))							\
			PANIC("VERIFY3( %s " #OP " %s ) "							\
				"failed (" FMT " " #OP " " FMT ")\n",					\
				#LEFT, #RIGHT,											\
				CAST (_verify3_left), CAST (_verify3_right));			\
	} while (0)


#define VERIFY3B(x,y,z)	VERIFY3_IMPL(x, y, z, int64_t, "%u", (boolean_t))
#define VERIFY3S(x,y,z)	VERIFY3_IMPL(x, y, z, int64_t, "%lld", (long long))
#define VERIFY3U(x,y,z)	VERIFY3_IMPL(x, y, z, uint64_t, "%llu",	\
		(unsigned long long))
#define VERIFY3P(x,y,z)	VERIFY3_IMPL(x, y, z, uintptr_t, "%p", (void *))
#define VERIFY0(x)	VERIFY3_IMPL(0, ==, x, int64_t, "%lld",	(long long))



#ifndef DEBUG /* Debugging Disabled */

/* Define SPL_DEBUG_STR to make clear which ASSERT definitions are used */
#define SPL_DEBUG_STR	""

#define ASSERT3B(x,y,z)	((void)0)
#define ASSERT3S(x,y,z)	((void)0)
#define ASSERT3U(x,y,z)	((void)0)
#define ASSERT3P(x,y,z)	((void)0)
#define ASSERT0(x)	((void)0)
#define ASSERT(x)	((void)0)
#define ASSERTV(x)

#define IMPLY(A, B) ((void)0)
#define EQUIV(A, B) ((void)0)




#else /* Debugging Enabled */




/* Define SPL_DEBUG_STR to make clear which ASSERT definitions are used */
#define SPL_DEBUG_STR	" (DEBUG mode)"


/* ASSERTION that will debug log used outside the debug system
 * Change the #if if you want ASSERTs to also call panic. We call
 * assfail() instead of printf() as to give a dtrace probe.
 */
extern int assfail(const char *str, const char *file, unsigned int line);

#if 1
#define PRINT printf
#else
#define PRINT panic
#endif

#define ASSERT(cond)												\
	(void)(unlikely(!(cond)) && assfail(#cond,__FILE__,__LINE__) &&	\
		PRINT("%s %s %d : %s\n", __FILE__, __FUNCTION__, __LINE__,	\
			"ASSERTION(" #cond ") failed\n"))

#define ASSERT3_IMPL(LEFT, OP, RIGHT, TYPE, FMT, CAST)				\
	do {															\
		if (!((TYPE)(LEFT) OP (TYPE)(RIGHT)) &&						\
			assfail(#LEFT #OP #RIGHT, __FILE__, __LINE__))			\
			PRINT("%s %s %d : ASSERT3( %s " #OP " %s) "				\
				"failed (" FMT " " #OP " " FMT ")\n",				\
				__FILE__, __FUNCTION__, __LINE__,					\
				#LEFT,	#RIGHT,										\
				CAST (LEFT), CAST (RIGHT));							\
	} while (0)


#define ASSERTF(cond, fmt, a...)								\
	do {														\
		if (unlikely(!(cond)))									\
			panic("ASSERTION(" #cond ") failed: " fmt, ## a);	\
	} while (0)


#define ASSERT3B(x,y,z)	ASSERT3_IMPL(x, y, z, int64_t, "%u", (boolean_t))
#define ASSERT3S(x,y,z)	ASSERT3_IMPL(x, y, z, int64_t, "%lld", (long long))
#define ASSERT3U(x,y,z)	ASSERT3_IMPL(x, y, z, uint64_t, "%llu",	(unsigned long long))

#define ASSERT3P(x,y,z)	ASSERT3_IMPL(x, y, z, uintptr_t, "%p", (void *))
#define ASSERT0(x)	ASSERT3_IMPL(0, ==, x, int64_t, "%lld", (long long))
#define ASSERTV(x)	x


/*
 * IMPLY and EQUIV are assertions of the form:
 *
 *      if (a) then (b)
 * and
 *      if (a) then (b) *AND* if (b) then (a)
 */
#define IMPLY(A, B)														\
	((void)(((!(A)) || (B)) ||											\
		printf("%s:%d (" #A ") implies (" #B "): failed\n",				\
			__FILE__, __LINE__)))

#define EQUIV(A, B)														\
	((void)((!!(A) == !!(B)) ||											\
		printf("%s:%d (" #A ") is equivalent to (" #B "): failed\n",	\
			__FILE__, __LINE__)))



#endif /* NDEBUG */


void spl_backtrace(char *thesignal);
int getpcstack(unsigned long *pcstack, int pcstack_limit);
void print_symbol(unsigned long symbol);


/*
 * Compile-time assertion. The condition 'x' must be constant.
 */
#define	CTASSERT(x)			_Static_assert((x), #x)

#ifdef __cplusplus
}
#endif

#endif /* SPL_DEBUG_H */
