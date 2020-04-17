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

#ifndef ZFS_SYS_SEMAPHORE_H_
#define	ZFS_SYS_SEMAPHORE_H_

/*
 * In macOS the semaphore calls sem_init, sem_destroy() are deprecated.
 * Implement a wrapper around it using pthread_condvar()
 */

typedef struct
{
    pthread_mutex_t count_lock;
    pthread_cond_t  count_bump;
    unsigned int count;
} sem_s;

typedef sem_s *sem_t;

static inline int sem_init(sem_t *psem, int flags, unsigned int count)
//static inline int sem_init(sem_s **psem, int flags, unsigned int count)
{
    sem_s *pnewsem;
    int result;

    pnewsem = (sem_s *)malloc(sizeof(sem_s));
    if (pnewsem == NULL)
        return -1;

    result = pthread_mutex_init(&pnewsem->count_lock, NULL);
    if (result != 0) {
        free(pnewsem);
        return result;
    }

    result = pthread_cond_init(&pnewsem->count_bump, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&pnewsem->count_lock);
        free(pnewsem);
        return result;
    }

    pnewsem->count = count;

    *psem = pnewsem;
    return 0;
}

static inline int sem_destroy(sem_t *psem)
{
    if (psem == NULL)
        return EINVAL;

	sem_s *isem = *psem;

    pthread_mutex_destroy(&isem->count_lock);
    pthread_cond_destroy(&isem->count_bump);
    free(psem);
    return 0;
}

static inline int sem_post(sem_t *psem)
{
    int result, xresult;

    if (psem == NULL)
        return EINVAL;

	sem_s *isem = *psem;

    result = pthread_mutex_lock(&isem->count_lock);
    if (result != 0)
        return result;

    isem->count = isem->count + 1;

    xresult = pthread_cond_signal(&isem->count_bump);

    result = pthread_mutex_unlock(&isem->count_lock);
    if (result != 0)
        return result;

    if (xresult != 0) {
        errno = xresult;
        return -1;
    }

	return 0;
}

static inline int sem_trywait(sem_t *psem)
{
    int result, xresult;

    if (psem == NULL)
        return EINVAL;

	sem_s *isem = *psem;

    result = pthread_mutex_lock(&isem->count_lock);
    if (result != 0)
        return result;

    xresult = 0;

    if (isem->count > 0)
        isem->count--;
    else
        xresult = EAGAIN;

    result = pthread_mutex_unlock(&isem->count_lock);
    if (result != 0)
        return result;

    if (xresult) {
        errno = xresult;
        return -1;
    }

    return 0;
}

static inline int sem_wait(sem_t *psem)
{
    int result, xresult;

    if (psem == NULL)
        return EINVAL;

	sem_s *isem = *psem;

    result = pthread_mutex_lock(&isem->count_lock);
    if (result != 0)
        return result;

    xresult = 0;

    if (isem->count == 0)
        xresult = pthread_cond_wait(&isem->count_bump, &isem->count_lock);

    if (xresult == 0)
        if (isem->count > 0)
            isem->count--;

    result = pthread_mutex_unlock(&isem->count_lock);
    if (result != 0)
        return result;

    if (xresult != 0) {
        errno = xresult;
        return -1;
    }

    return 0;
}

static inline int sem_timedwait(sem_t *psem, const struct timespec *abstim)
{
    int result, xresult;

    if (psem == NULL)
        return EINVAL;

	sem_s *isem = *psem;

    result = pthread_mutex_lock(&isem->count_lock);
    if (result != 0)
        return result;

    xresult = 0;

    if (isem->count == 0)
        xresult = pthread_cond_timedwait(&isem->count_bump,
			&isem->count_lock, abstim);

    if (xresult == 0)
        if (isem->count > 0)
            isem->count--;

    result = pthread_mutex_unlock(&isem->count_lock);
    if (result != 0)
        return result;

    if (xresult != 0) {
        errno = xresult;
        return -1;
    }

    return 0;
}

#endif /* !ZFS_SYS_SEMAPHORE_H_ */


