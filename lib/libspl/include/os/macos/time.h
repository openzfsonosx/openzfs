/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
#ifndef _LIBSPL_TIME_H
#define	_LIBSPL_TIME_H

#include_next <time.h>

#include <TargetConditionals.h>
#include <AvailabilityMacros.h>

/* Linux also has a timer_create() API we need to emulate. */

#include <dlfcn.h>
#include <sys/stdtypes.h>
#include <stdbool.h>
#include <mach/boolean.h>
#include <sys/errno.h>
#include <stdlib.h>

#include <dispatch/dispatch.h>

/*
 * Unfortunately; Linux defines "bool" in struct fmd_stat, which is a
 * reserved name. Work around it.
 */
#ifndef  __cplusplus
#undef bool
typedef int bool;
#endif

#if !defined(MAC_OS_X_VERSION_10_12) || \
	(MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_12)
typedef int clockid_t;
#endif

struct itimerspec {
	struct timespec it_interval;	/* timer period */
	struct timespec it_value;		/* timer expiration */
};

struct sigevent;

/* If used a lot, queue should probably be outside of this struct */
struct macos_timer {
	dispatch_queue_t tim_queue;
	dispatch_source_t tim_timer;
	void (*tim_func)(union sigval);
	void *tim_arg;
};

typedef struct macos_timer *timer_t;

static inline void
_timer_cancel(void *arg)
{
	struct macos_timer *tim = (struct macos_timer *)arg;
	dispatch_release(tim->tim_timer);
	dispatch_release(tim->tim_queue);
	tim->tim_timer = NULL;
	tim->tim_queue = NULL;
	free(tim);
}

static inline void
_timer_handler(void *arg)
{
	struct macos_timer *tim = (struct macos_timer *)arg;
	union sigval sv;

	sv.sival_ptr = tim->tim_arg;

	if (tim->tim_func != NULL)
		tim->tim_func(sv);
}

static inline int
timer_create(clockid_t clockid, struct sigevent *sevp,
    timer_t *timerid)
{
	struct macos_timer *tim;

	*timerid = NULL;

	switch (clockid) {
		case CLOCK_REALTIME:

			/* What is implemented so far */
			if (sevp->sigev_notify != SIGEV_THREAD) {
				errno = ENOTSUP;
				return (-1);
			}

			tim = (struct macos_timer *)
			    malloc(sizeof (struct macos_timer));
			if (tim == NULL) {
				errno = ENOMEM;
				return (-1);
			}

			tim->tim_queue =
			    dispatch_queue_create("org.openzfsonosx.timerqueue",
			    0);
			tim->tim_timer =
			    dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
			    0, 0, tim->tim_queue);

			tim->tim_func = sevp->sigev_notify_function;
			tim->tim_arg = sevp->sigev_value.sival_ptr;
			*timerid = tim;

			/* Opting to use pure C instead of Block versions */
			dispatch_set_context(tim->tim_timer, tim);
			dispatch_source_set_event_handler_f(tim->tim_timer,
			    _timer_handler);
			dispatch_source_set_cancel_handler_f(tim->tim_timer,
			    _timer_cancel);

			return (0);
		default:
			break;
	}

	errno = EINVAL;
	return (-1);
}

static inline int
timer_settime(timer_t tim, int flags,
    const struct itimerspec *its, struct itimerspec *remainvalue)
{
	if (tim != NULL) {

		/* Both zero, is disarm */
		if (its->it_value.tv_sec == 0 &&
		    its->it_value.tv_nsec == 0) {
		/* There's a comment about suspend count in Apple docs */
			dispatch_suspend(tim->tim_timer);
			return (0);
		}

		dispatch_time_t start;
		start = dispatch_time(DISPATCH_TIME_NOW,
		    NSEC_PER_SEC * its->it_value.tv_sec +
		    its->it_value.tv_nsec);
		dispatch_source_set_timer(tim->tim_timer, start,
		    NSEC_PER_SEC * its->it_value.tv_sec +
		    its->it_value.tv_nsec,
		    0);
		dispatch_resume(tim->tim_timer);
	}
	return (0);
}

static inline int
timer_delete(timer_t tim)
{
	/* Calls _timer_cancel() */
	if (tim != NULL)
		dispatch_source_cancel(tim->tim_timer);

	return (0);
}

#endif
