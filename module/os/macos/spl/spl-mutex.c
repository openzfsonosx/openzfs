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

#include <sys/mutex.h>
#include <sys/atomic.h>
#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <kern/thread.h>
#include <string.h>
#include <sys/debug.h>
#include <kern/debug.h>
#include <sys/thread.h>

// Not defined in headers
extern boolean_t lck_mtx_try_lock(lck_mtx_t *lck);


static lck_attr_t       *zfs_lock_attr = NULL;
static lck_grp_attr_t   *zfs_group_attr = NULL;

static lck_grp_t *zfs_mutex_group = NULL;

uint64_t zfs_active_mutex = 0;

#ifdef SPL_DEBUG_MUTEX
#include <sys/list.h>
static list_t mutex_list;
static kmutex_t mutex_list_mutex;


struct leak {
    list_node_t     mutex_leak_node;

#define SPL_DEBUG_MUTEX_MAXCHAR 32
	char location_file[SPL_DEBUG_MUTEX_MAXCHAR];
	char location_function[SPL_DEBUG_MUTEX_MAXCHAR];
	uint64_t location_line;
	void *mp;

	uint64_t     wdlist_locktime;       // time lock was taken
	char         wdlist_file[32];  // storing holder
	uint64_t     wdlist_line;
};

static int wdlist_exit = 0;

void spl_wdlist_settime(void *mpleak, uint64_t value)
{
	struct leak *leak = (struct leak *)mpleak;
	if (!leak) return;
	leak->wdlist_locktime = value;
}

inline static void spl_wdlist_check(void *ignored)
{
	struct leak *mp;
	printf("SPL: Mutex watchdog is alive\n");

	while(!wdlist_exit) {
		delay(hz*SPL_MUTEX_WATCHDOG_SLEEP);
		uint64_t noe = gethrestime_sec();
		lck_mtx_lock((lck_mtx_t *)&mutex_list_mutex.m_lock);
		for (mp = list_head(&mutex_list);
			 mp;
			 mp = list_next(&mutex_list, mp)) {
			uint64_t locktime = mp->wdlist_locktime;
			if ((locktime > 0) && (noe > locktime) &&
				noe - locktime >= SPL_MUTEX_WATCHDOG_TIMEOUT) {
				printf("SPL: mutex (%p) held for %llus by '%s':%llu\n",
					   mp, noe - mp->wdlist_locktime, mp->wdlist_file, mp->wdlist_line);
			} // if old
		} // for all
		lck_mtx_unlock((lck_mtx_t *)&mutex_list_mutex.m_lock);
    }// while not exit

	printf("SPL: watchdog thread exit\n");
	wdlist_exit = 2;
	thread_exit();
}


#endif


int spl_mutex_subsystem_init(void)
{
    zfs_lock_attr = lck_attr_alloc_init();
    zfs_group_attr = lck_grp_attr_alloc_init();
    zfs_mutex_group  = lck_grp_alloc_init("zfs-mutex", zfs_group_attr);


#ifdef SPL_DEBUG_MUTEX
	{
		unsigned char mutex[128];
		int i;

		memset(mutex, 0xAF, sizeof(mutex));
		lck_mtx_init((lck_mtx_t *)&mutex[0], zfs_mutex_group, zfs_lock_attr);
		for (i = sizeof(mutex)-1; i >=0 ; i--)
			if (mutex[i] != 0xAF) break;

		printf("SPL: mutex size is %u\n", i+1);

	}

	list_create(&mutex_list, sizeof (struct leak),
				offsetof(struct leak, mutex_leak_node));
	lck_mtx_init((lck_mtx_t *)&mutex_list_mutex.m_lock, zfs_mutex_group, zfs_lock_attr);

	(void)thread_create(NULL, 0, spl_wdlist_check, 0, 0, 0, 0, 92);
#endif
	return 0;
}



void spl_mutex_subsystem_fini(void)
{
#ifdef SPL_DEBUG_MUTEX
	uint64_t total = 0;
	printf("Dumping leaked mutex allocations...\n");

	wdlist_exit = 1;

	mutex_enter(&mutex_list_mutex);
	while(1) {
		struct leak *leak, *runner;
		uint32_t found;

		leak = list_head(&mutex_list);

		if (leak) {
			list_remove(&mutex_list, leak);
		}
		if (!leak) break;

		// Run through list and count up how many times this leak is
		// found, removing entries as we go.
		for (found = 1, runner = list_head(&mutex_list);
			 runner;
			 runner = runner ? list_next(&mutex_list, runner) :
				 list_head(&mutex_list)) {

			if (!strcmp(leak->location_file, runner->location_file) &&
				!strcmp(leak->location_function, runner->location_function) &&
				leak->location_line == runner->location_line) {
				// Same place
				found++;
				list_remove(&mutex_list, runner);
				FREE(runner, M_TEMP);
				runner = NULL;
			} // if same

		} // for all nodes

		printf("  mutex %p : %s %s %llu : # leaks: %u\n",
			   leak->mp,
			   leak->location_file,
			   leak->location_function,
			   leak->location_line,
			   found);

		FREE(leak, M_TEMP);
		total+=found;

	}
	mutex_exit(&mutex_list_mutex);
	printf("Dumped %llu leaked allocations. Wait for watchdog to exit..\n", total);

	while(wdlist_exit != 2) delay(hz>>4);

	lck_mtx_destroy((lck_mtx_t *)&mutex_list_mutex.m_lock, zfs_mutex_group);
	list_destroy(&mutex_list);
#endif

    lck_attr_free(zfs_lock_attr);
    zfs_lock_attr = NULL;

    lck_grp_attr_free(zfs_group_attr);
    zfs_group_attr = NULL;

    lck_grp_free(zfs_mutex_group);
    zfs_mutex_group = NULL;
}



#ifdef SPL_DEBUG_MUTEX
void spl_mutex_init(kmutex_t *mp, char *name, kmutex_type_t type, void *ibc,
					const char *file, const char *fn, int line)
#else
void spl_mutex_init(kmutex_t *mp, char *name, kmutex_type_t type, void *ibc)
#endif
{
    ASSERT(type != MUTEX_SPIN);
    ASSERT(ibc == NULL);
	lck_mtx_init((lck_mtx_t *)&mp->m_lock, zfs_mutex_group, zfs_lock_attr);
    mp->m_owner = NULL;

	atomic_inc_64(&zfs_active_mutex);

#ifdef SPL_DEBUG_MUTEX
	struct leak *leak;

	MALLOC(leak, struct leak *,
		   sizeof(struct leak),  M_TEMP, M_WAITOK);

	if (leak) {
		bzero(leak, sizeof(struct leak));
		strlcpy(leak->location_file, file, SPL_DEBUG_MUTEX_MAXCHAR);
		strlcpy(leak->location_function, fn, SPL_DEBUG_MUTEX_MAXCHAR);
		leak->location_line = line;
		leak->mp = mp;

		mutex_enter(&mutex_list_mutex);
		list_link_init(&leak->mutex_leak_node);
		list_insert_tail(&mutex_list, leak);
		mp->leak = leak;
		mutex_exit(&mutex_list_mutex);
	}
	leak->wdlist_locktime = 0;
	leak->wdlist_file[0] = 0;
	leak->wdlist_line = 0;
#endif
}

void spl_mutex_destroy(kmutex_t *mp)
{
    if (!mp) return;

	if (mp->m_owner != 0) panic("SPL: releasing held mutex");

	lck_mtx_destroy((lck_mtx_t *)&mp->m_lock, zfs_mutex_group);

	atomic_dec_64(&zfs_active_mutex);

#ifdef SPL_DEBUG_MUTEX
	if (mp->leak) {
		struct leak *leak = (struct leak *)mp->leak;
		mutex_enter(&mutex_list_mutex);
		list_remove(&mutex_list, leak);
		mp->leak = NULL;
		mutex_exit(&mutex_list_mutex);
		FREE(leak, M_TEMP);
	}
#endif
}



#ifdef SPL_DEBUG_MUTEX
void spl_mutex_enter(kmutex_t *mp, char *file, int line)
#else
void spl_mutex_enter(kmutex_t *mp)
#endif
{
    if (mp->m_owner == current_thread())
        panic("mutex_enter: locking against myself!");

#ifdef DEBUG
	if (*((uint64_t *)mp) == 0xdeadbeefdeadbeef) {
		panic("SPL: mutex_enter");
	}
#endif

    lck_mtx_lock((lck_mtx_t *)&mp->m_lock);
    mp->m_owner = current_thread();

#ifdef SPL_DEBUG_MUTEX
	if (mp->leak) {
		struct leak *leak = (struct leak *)mp->leak;
		leak->wdlist_locktime = gethrestime_sec();
		strlcpy(leak->wdlist_file, file, sizeof(leak->wdlist_file));
		leak->wdlist_line = line;
	}
#endif

}

void spl_mutex_exit(kmutex_t *mp)
{
#ifdef DEBUG
	if (*((uint64_t *)mp) == 0xdeadbeefdeadbeef) {
		panic("SPL: mutex_exit");
	}
#endif

#ifdef SPL_DEBUG_MUTEX
	if (mp->leak) {
		struct leak *leak = (struct leak *)mp->leak;
		uint64_t locktime = leak->wdlist_locktime;
		uint64_t noe = gethrestime_sec();
		if ((locktime > 0) && (noe > locktime) &&
			noe - locktime >= SPL_MUTEX_WATCHDOG_TIMEOUT) {
			printf("SPL: mutex (%p) finally released after %llus by '%s':%llu\n",
				   leak, noe - leak->wdlist_locktime, leak->wdlist_file,
				   leak->wdlist_line);
		}
		leak->wdlist_locktime = 0;
		leak->wdlist_file[0] = 0;
		leak->wdlist_line = 0;
	}
#endif
    mp->m_owner = NULL;
    lck_mtx_unlock((lck_mtx_t *)&mp->m_lock);
}


int spl_mutex_tryenter(kmutex_t *mp)
{
    int held;

    if (mp->m_owner == current_thread())
        panic("mutex_tryenter: locking against myself!");

    held = lck_mtx_try_lock((lck_mtx_t *)&mp->m_lock);
    if (held) {
        mp->m_owner = current_thread();

#ifdef SPL_DEBUG_MUTEX
	if (mp->leak) {
		struct leak *leak = (struct leak *)mp->leak;
		leak->wdlist_locktime = gethrestime_sec();
		strlcpy(leak->wdlist_file, "tryenter", sizeof(leak->wdlist_file));
		leak->wdlist_line = 123;
	}
#endif

	}
	return (held);
}

int spl_mutex_owned(kmutex_t *mp)
{
    return (mp->m_owner == current_thread());
}

struct kthread *spl_mutex_owner(kmutex_t *mp)
{
    return (mp->m_owner);
}
