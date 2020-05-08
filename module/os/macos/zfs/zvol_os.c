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
 * Copyright (c) 2020 by Jorgen Lundman. All rights reserved.
 */

#include <sys/dataset_kstats.h>
#include <sys/disk.h>
#include <sys/dbuf.h>
#include <sys/dmu_traverse.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_dir.h>
#include <sys/zap.h>
#include <sys/zfeature.h>
#include <sys/zil_impl.h>
#include <sys/dmu_tx.h>
#include <sys/zio.h>
#include <sys/zfs_rlock.h>
#include <sys/spa_impl.h>
#include <sys/zvol.h>
#include <sys/zvol_impl.h>

unsigned int zvol_major = ZVOL_MAJOR;
unsigned int zvol_request_sync = 0;
unsigned int zvol_prefetch_bytes = (128 * 1024);
unsigned long zvol_max_discard_blocks = 16384;
unsigned int zvol_threads = 8;

struct zvol_state_os {

	dataset_kstats_t	zvo_kstat;	/* zvol kstats */
	dev_t			zvo_dev;	/* device id */
};

taskq_t *zvol_taskq;

typedef struct zv_request {
	zvol_state_t	*zv;

	taskq_ent_t	ent;
} zv_request_t;

/*
 * Given a path, return TRUE if path is a ZVOL.
 */
static boolean_t
zvol_os_is_zvol(const char *device)
{

	return (B_FALSE);
}

int
zvol_os_write(dev_t dev, struct uio *uio, int p)
{

}

static void
zvol_os_discard(void *arg)
{

}

int
zvol_os_read(dev_t dev, struct uio *uio, int p)
{

}



int
zvol_os_update_volsize(zvol_state_t *zv, uint64_t volsize)
{

	return (0);
}

static void
zvol_os_clear_private(zvol_state_t *zv)
{

}

int
zvol_os_open(dev_t devp, int flag, int otyp, struct proc *p)
{
	if (!getminor(devp))
		return (0);

	return -1;
}

int
zvol_os_close(dev_t dev, int flag, int otyp, struct proc *p)
{
	minor_t minor = getminor(dev);

	if (!getminor(dev))
		return (0);

	return 0;
}

void
zvol_os_strategy(struct buf *bp)
{

}

/*
 * Allocate memory for a new zvol_state_t and setup the required
 * request queue and generic disk structures for the block device.
 */
static zvol_state_t *
zvol_os_alloc(dev_t dev, const char *name)
{
	return (NULL);
}

/*
 * Cleanup then free a zvol_state_t which was created by zvol_alloc().
 * At this time, the structure is not opened by anyone, is taken off
 * the zvol_state_list, and has its private data set to NULL.
 * The zvol_state_lock is dropped.
 */
static void
zvol_os_free(zvol_state_t *zv)
{

}

/*
 * Create a block device minor node and setup the linkage between it
 * and the specified volume.  Once this function returns the block
 * device is live and ready for use.
 */
static int
zvol_os_create_minor(const char *name)
{
	int error = 0;

	return (error);
}

static void
zvol_os_rename_minor(zvol_state_t *zv, const char *newname)
{

}

static void
zvol_os_set_disk_ro(zvol_state_t *zv, int flags)
{

}

static void
zvol_os_set_capacity(zvol_state_t *zv, uint64_t capacity)
{

}

int
zvol_os_get_volume_blocksize(dev_t dev)
{
	/* XNU can only handle two sizes. */
	return (DEV_BSIZE);
}

int
zvol_os_ioctl(dev_t dev, unsigned long cmd, caddr_t data, int isblk,
    cred_t *cr, int *rvalp)
{
	int error = 0;
	u_int32_t *f;
	u_int64_t *o;
	zvol_state_t *zv = NULL;

	if (!getminor(dev))
		return (ENXIO);

	rw_enter(&zvol_state_lock, RW_READER);
//	zv = zfsdev_get_soft_state(getminor(dev), ZSST_ZVOL);
	rw_exit(&zvol_state_lock);

	if (zv == NULL) {
		dprintf("zv is NULL\n");
		return (ENXIO);
	}

	f = (u_int32_t *)data;
	o = (u_int64_t *)data;

	switch (cmd) {

		case DKIOCGETMAXBLOCKCOUNTREAD:
			dprintf("DKIOCGETMAXBLOCKCOUNTREAD\n");
			*o = 32;
			break;

		case DKIOCGETMAXBLOCKCOUNTWRITE:
			dprintf("DKIOCGETMAXBLOCKCOUNTWRITE\n");
			*o = 32;
			break;
		case DKIOCGETMAXSEGMENTCOUNTREAD:
			dprintf("DKIOCGETMAXSEGMENTCOUNTREAD\n");
			*o = 32;
			break;

		case DKIOCGETMAXSEGMENTCOUNTWRITE:
			dprintf("DKIOCGETMAXSEGMENTCOUNTWRITE\n");
			*o = 32;
			break;

		case DKIOCGETBLOCKSIZE:
			dprintf("DKIOCGETBLOCKSIZE: %llu\n",
				zv->zv_volblocksize);
			*f = zv->zv_volblocksize;
			break;

		case DKIOCSETBLOCKSIZE:
			dprintf("DKIOCSETBLOCKSIZE %lu\n", *f);

			if (!isblk) {
				/* We can only do this for a block device */
				error = ENODEV;
				break;
			}

			if (zvol_check_volblocksize(zv->zv_name, (uint64_t)*f)) {
				error = EINVAL;
				break;
			}

			/* set the new block size */
			zv->zv_volblocksize = (uint64_t)*f;
			dprintf("setblocksize changed: %llu\n",
				zv->zv_volblocksize);
			break;

		case DKIOCISWRITABLE:
			dprintf("DKIOCISWRITABLE\n");
			if (zv && (zv->zv_flags & ZVOL_RDONLY))
				*f = 0;
			else
				*f = 1;
			break;
#ifdef DKIOCGETBLOCKCOUNT32
		case DKIOCGETBLOCKCOUNT32:
			dprintf("DKIOCGETBLOCKCOUNT32: %lu\n",
				(uint32_t)zv->zv_volsize / zv->zv_volblocksize);
			*f = (uint32_t)zv->zv_volsize / zv->zv_volblocksize;
			break;
#endif

		case DKIOCGETBLOCKCOUNT:
			dprintf("DKIOCGETBLOCKCOUNT: %llu\n",
				zv->zv_volsize / zv->zv_volblocksize);
			*o = (uint64_t)zv->zv_volsize / zv->zv_volblocksize;
			break;

		case DKIOCGETBASE:
			dprintf("DKIOCGETBASE\n");
			/*
			 * What offset should we say?
			 * 0 is ok for FAT but to HFS
			 */
			*o = zv->zv_volblocksize * 0;
			break;

		case DKIOCGETPHYSICALBLOCKSIZE:
			dprintf("DKIOCGETPHYSICALBLOCKSIZE\n");
			*f = zv->zv_volblocksize;
			break;

#ifdef DKIOCGETTHROTTLEMASK
		case DKIOCGETTHROTTLEMASK:
			dprintf("DKIOCGETTHROTTLEMASK\n");
			*o = 0;
			break;
#endif

		case DKIOCGETMAXBYTECOUNTREAD:
			*o = SPA_MAXBLOCKSIZE;
			break;

		case DKIOCGETMAXBYTECOUNTWRITE:
			*o = SPA_MAXBLOCKSIZE;
			break;
#ifdef DKIOCUNMAP
		case DKIOCUNMAP:
			dprintf("DKIOCUNMAP\n");
			*f = 1;
			break;
#endif

		case DKIOCGETFEATURES:
			*f = 0;
			break;

#ifdef DKIOCISSOLIDSTATE
		case DKIOCISSOLIDSTATE:
			dprintf("DKIOCISSOLIDSTATE\n");
			*f = 0;
			break;
#endif

		case DKIOCISVIRTUAL:
			*f = 1;
			break;

		case DKIOCGETMAXSEGMENTBYTECOUNTREAD:
			*o = 32 * zv->zv_volblocksize;
			break;

		case DKIOCGETMAXSEGMENTBYTECOUNTWRITE:
			*o = 32 * zv->zv_volblocksize;
			break;

		case DKIOCSYNCHRONIZECACHE:
			dprintf("DKIOCSYNCHRONIZECACHE\n");
			break;

		default:
			dprintf("unknown ioctl: ENOTTY\n");
			error = ENOTTY;
			break;
	}

	return (SET_ERROR(error));
}

const static zvol_platform_ops_t zvol_macos_ops = {
	.zv_free = zvol_os_free,
	.zv_rename_minor = zvol_os_rename_minor,
	.zv_create_minor = zvol_os_create_minor,
	.zv_update_volsize = zvol_os_update_volsize,
	.zv_clear_private = zvol_os_clear_private,
	.zv_is_zvol = zvol_os_is_zvol,
	.zv_set_disk_ro = zvol_os_set_disk_ro,
	.zv_set_capacity = zvol_os_set_capacity,
};

int
zvol_init(void)
{
	int threads = MIN(MAX(zvol_threads, 1), 1024);

	zvol_taskq = taskq_create(ZVOL_DRIVER, threads, maxclsyspri,
	    threads * 2, INT_MAX, TASKQ_PREPOPULATE | TASKQ_DYNAMIC);
	if (zvol_taskq == NULL) {
		return (-ENOMEM);
	}

	zvol_init_impl();
	zvol_register_ops(&zvol_macos_ops);
	return (0);
}

void
zvol_fini(void)
{
	zvol_fini_impl();
	taskq_destroy(zvol_taskq);
}

