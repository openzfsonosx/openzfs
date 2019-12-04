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
 * Copyright (C) 2013 Jorgen Lundman <lundman@lundman.net>
 *
 */

#include <sys/condvar.h>
#include <sys/errno.h>
#include <sys/callb.h>

#ifdef SPL_DEBUG_MUTEX
void spl_wdlist_settime(void *mpleak, uint64_t value);
#endif

void
spl_cv_init(kcondvar_t *cvp, char *name, kcv_type_t type, void *arg)
{
}

void
spl_cv_destroy(kcondvar_t *cvp)
{
}

void
spl_cv_signal(kcondvar_t *cvp)
{
    wakeup_one((caddr_t)cvp);
}

void
spl_cv_broadcast(kcondvar_t *cvp)
{
    wakeup((caddr_t)cvp);
}


/*
 * Block on the indicated condition variable and
 * release the associated mutex while blocked.
 */
void
spl_cv_wait(kcondvar_t *cvp, kmutex_t *mp, int flags, const char *msg)
{
    if (msg != NULL && msg[0] == '&')
        ++msg;  /* skip over '&' prefixes */

#ifdef SPL_DEBUG_MUTEX
	spl_wdlist_settime(mp->leak, 0);
#endif
	mp->m_owner = NULL;
    (void) msleep(cvp, (lck_mtx_t *)&mp->m_lock, flags, msg, 0);
    mp->m_owner = current_thread();
#ifdef SPL_DEBUG_MUTEX
	spl_wdlist_settime(mp->leak, gethrestime_sec());
#endif
}

/*
 * Same as cv_wait except the thread will unblock at 'tim'
 * (an absolute time) if it hasn't already unblocked.
 *
 * Returns the amount of time left from the original 'tim' value
 * when it was unblocked.
 */
int
spl_cv_timedwait(kcondvar_t *cvp, kmutex_t *mp, clock_t tim, int flags,
				 const char *msg)
{
    struct timespec ts;
    int result;
	uint64_t timenow;

    if (msg != NULL && msg[0] == '&')
        ++msg;  /* skip over '&' prefixes */

	timenow = zfs_lbolt();

	// Check for events already in the past
	if (tim < timenow)
		return -1; // timedout

	// Compute the delta
	tim = tim - timenow;

	// figure out sec and nsec
	ts.tv_sec = (tim / hz);
    ts.tv_nsec = (tim % hz) * NSEC_PER_SEC / hz;

	// Both sec and nsec zero is a blocking call. (Not poll)
	if (ts.tv_sec == 0 &&
		ts.tv_nsec == 0)
		ts.tv_nsec = 1000;

	// Sanity check
    if (ts.tv_sec > 400) {
        printf("cv_timedwait: would have waited %lds\n", ts.tv_sec);
		ts.tv_sec = 5;
	}
#ifdef SPL_DEBUG_MUTEX
	spl_wdlist_settime(mp->leak, 0);
#endif
    mp->m_owner = NULL;
    result = msleep(cvp, (lck_mtx_t *)&mp->m_lock, flags, msg, &ts);

	// msleep grabs the mutex, even if timeout/signal
	mp->m_owner = current_thread();

#ifdef SPL_DEBUG_MUTEX
	spl_wdlist_settime(mp->leak, gethrestime_sec());
#endif
	return (result == EWOULDBLOCK ? -1 : 0);
}


/*
* Compatibility wrapper for the cv_timedwait_hires() Illumos interface.
*/
clock_t
cv_timedwait_hires(kcondvar_t *cvp, kmutex_t *mp, hrtime_t tim,
                 hrtime_t res, int flag)
{
    struct timespec ts;
    int result;
	hrtime_t time_left;

    if (res > 1) {
        /*
         * Align expiration to the specified resolution.
         */
        if (flag & CALLOUT_FLAG_ROUNDUP)
            tim += res - 1;
        tim = (tim / res) * res;
    }

    if ((flag & CALLOUT_FLAG_ABSOLUTE)) {
		time_left = tim - gethrtime();
		if (time_left <= 0)
			return (-1);
		tim = time_left;
	}

    ts.tv_sec = 0;
    ts.tv_nsec = tim;

    if (ts.tv_nsec <= 100) {
		printf("cv_timedwait_hires: warning, sleep is less that 100nsec %lds\n",
			   ts.tv_nsec);
        ts.tv_nsec = 1000; // At least one microsecond
	}

    if (ts.tv_nsec > 400L * NSEC_PER_SEC) {
        printf("cv_timedwait_hires: will wait %llus -> forced to 5s\n",
			   (uint64_t)ts.tv_nsec/NSEC_PER_SEC);
		ts.tv_nsec = 5 * NSEC_PER_SEC;
	}

#ifdef SPL_DEBUG_MUTEX
	spl_wdlist_settime(mp->leak, 0);
#endif
    mp->m_owner = NULL;
    result = msleep(cvp, (lck_mtx_t *)&mp->m_lock, PRIBIO, "cv_timedwait_hires", &ts);
    mp->m_owner = current_thread();
#ifdef SPL_DEBUG_MUTEX
	spl_wdlist_settime(mp->leak, gethrestime_sec());
#endif

    return (result == EWOULDBLOCK ? -1 : 0);

}
