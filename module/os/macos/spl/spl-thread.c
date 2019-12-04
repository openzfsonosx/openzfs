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
 *
 * Copyright (C) 2008 MacZFS
 * Copyright (C) 2013 Jorgen Lundman <lundman@lundman.net>
 *
 */

#include <sys/thread.h>
#include <mach/thread_act.h>
#include <sys/kmem.h>
#include <sys/tsd.h>
#include <sys/debug.h>
#include <sys/vnode.h>
#include <sys/callb.h>
#include <sys/systm.h>

uint64_t zfs_threads = 0;

kthread_t *
spl_thread_create(
        caddr_t         stk,
        size_t          stksize,
        void            (*proc)(void *),
        void            *arg,
        size_t          len,
        /*struct proc     *pp,*/
        int             state,
#ifdef SPL_DEBUG_THREAD
		char *filename,
		int line,
#endif
        pri_t           pri)
{
        kern_return_t   result;
        thread_t        thread;

#ifdef SPL_DEBUG_THREAD
		printf("Start thread pri %d by '%s':%d\n", pri,
			   filename, line);
#endif

        result= kernel_thread_start((thread_continue_t)proc, arg, &thread);

        if (result != KERN_SUCCESS)
			return (NULL);

		/* Improve the priority when asked to do so */
		if (pri > minclsyspri) {
			thread_precedence_policy_data_t policy;

			/* kernel priorities (osfmk/kern/sched.h)
			 *
			 * 96           Reserved (real-time)
			 * 95           Kernel mode only
			 *                              A
			 *                              +
			 *                      (16 levels)
			 *                              +
			 *                              V
			 * 80           Kernel mode only
			 * 79           System high priority
			 *
			 * spl/include/sys/sysmacros.h
			 * #define maxclsyspri  89
			 * #define minclsyspri  81  BASEPRI_KERNEL
			 * #define defclsyspri  81  BASEPRI_KERNEL
			 *
			 * Calling policy.importance = 10 will create
			 * a default pri (81) at pri (91).
			 *
			 * So asking for pri (85) we do 85-81 = 4.
			 *
q			 * IllumOS priorities are:
			 * #define MAXCLSYSPRI     99
			 * #define MINCLSYSPRI     60
			 */

			policy.importance = (pri - minclsyspri);

			thread_policy_set(thread,
							  THREAD_PRECEDENCE_POLICY,
							  (thread_policy_t)&policy,
							  THREAD_PRECEDENCE_POLICY_COUNT);
		}

        thread_deallocate(thread);

        atomic_inc_64(&zfs_threads);

        return ((kthread_t *)thread);
}

kthread_t *
spl_current_thread(void)
{
    thread_t cur_thread = current_thread();
    return ((kthread_t *)cur_thread);
}

void spl_thread_exit(void)
{
        atomic_dec_64(&zfs_threads);

		tsd_thread_exit();
        (void) thread_terminate(current_thread());
}


/*
 * IllumOS has callout.c - place it here until we find a better place
 */
callout_id_t
timeout_generic(int type, void (*func)(void *), void *arg,
				hrtime_t expiration, hrtime_t resolution, int flags)
{
	struct timespec ts;
	hrt2ts(expiration, &ts);
	bsd_timeout(func, arg, &ts);
	/* bsd_untimeout() requires func and arg to cancel the timeout, so
	 * pass it back as the callout_id. If we one day were to implement
	 * untimeout_generic() they would pass it back to us
	 */
	return (callout_id_t)arg;
}
