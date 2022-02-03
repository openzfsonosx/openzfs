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
 * Copyright 2014, 2020 Jorgen Lundman <lundman@lundman.net>
 */

#include <sys/spa.h>
#include <sys/zio.h>
#include <sys/zio_compress.h>
#include <sys/zfs_context.h>
#include <sys/arc.h>
#include <sys/zfs_refcount.h>
#include <sys/vdev.h>
#include <sys/vdev_impl.h>
#include <sys/dsl_pool.h>
#ifdef _KERNEL
#include <sys/vmsystm.h>
#endif
#include <sys/callb.h>
#include <sys/kstat.h>
#include <sys/kstat_osx.h>
#include <sys/zfs_ioctl.h>
#include <sys/spa.h>
#include <sys/zap_impl.h>
#include <sys/zil.h>
#include <sys/spa_impl.h>
#include <sys/crypto/icp.h>
#include <sys/vdev_raidz.h>
#include <zfs_fletcher.h>

/*
 * In Solaris the tunable are set via /etc/system. Until we have a load
 * time configuration, we add them to writable kstat tunables.
 *
 * This table is more or less populated from IllumOS mdb zfs_params sources
 * https://github.com/illumos/illumos-gate/blob/master/
 * usr/src/cmd/mdb/common/modules/zfs/zfs.c#L336-L392
 *
 * The content here should be moved into sysctl_os.c and
 * this file should be retired.
 */


osx_kstat_t osx_kstat = {
	{ "spa_version",			KSTAT_DATA_UINT64 },
	{ "zpl_version",			KSTAT_DATA_UINT64 },

	{ "active_vnodes",			KSTAT_DATA_UINT64 },
	{ "vnop_debug",				KSTAT_DATA_UINT64 },
	{ "reclaim_nodes",			KSTAT_DATA_UINT64 },
	{ "ignore_negatives",			KSTAT_DATA_UINT64 },
	{ "ignore_positives",			KSTAT_DATA_UINT64 },
	{ "create_negatives",			KSTAT_DATA_UINT64 },
	{ "force_formd_normalized",		KSTAT_DATA_UINT64 },
	{ "skip_unlinked_drain",		KSTAT_DATA_UINT64 },
	{ "use_system_sync",			KSTAT_DATA_UINT64 },

	{"zfs_vdev_raidz_impl",			KSTAT_DATA_STRING  },
	{"icp_gcm_impl",			KSTAT_DATA_STRING  },
	{"icp_aes_impl",			KSTAT_DATA_STRING  },
	{"zfs_fletcher_4_impl",			KSTAT_DATA_STRING  },

	{"zfs_expire_snapshot",			KSTAT_DATA_UINT64  },
	{"zfs_admin_snapshot",			KSTAT_DATA_UINT64  },
	{"zfs_auto_snapshot",			KSTAT_DATA_UINT64  },
	{"zfs_disable_spotlight",			KSTAT_DATA_UINT64  },
	{"zfs_disable_trashes",			KSTAT_DATA_UINT64  },
	{"zfs_dbgmsg_enable",			KSTAT_DATA_UINT64  },
	{"zfs_dbgmsg_maxsize",			KSTAT_DATA_UINT64  },
};

static char vdev_raidz_string[KSTAT_STRLEN] = { 0 };
static char icp_gcm_string[KSTAT_STRLEN] = { 0 };
static char icp_aes_string[KSTAT_STRLEN] = { 0 };
static char zfs_fletcher_4_string[KSTAT_STRLEN] = { 0 };

static kstat_t		*osx_kstat_ksp;

#if !defined(__OPTIMIZE__)
#pragma GCC diagnostic ignored "-Wframe-larger-than="
#endif

extern kstat_t *arc_ksp;

static int osx_kstat_update(kstat_t *ksp, int rw)
{
	osx_kstat_t *ks = ksp->ks_data;

	if (rw == KSTAT_WRITE) {

		/* Darwin */

		zfs_vnop_ignore_negatives =
		    ks->darwin_ignore_negatives.value.ui64;
		zfs_vnop_ignore_positives =
		    ks->darwin_ignore_positives.value.ui64;
		zfs_vnop_create_negatives =
		    ks->darwin_create_negatives.value.ui64;
		zfs_vnop_force_formd_normalized_output =
		    ks->darwin_force_formd_normalized.value.ui64;
		zfs_vnop_skip_unlinked_drain =
		    ks->darwin_skip_unlinked_drain.value.ui64;
		zfs_vfs_sync_paranoia =
		    ks->darwin_use_system_sync.value.ui64;

		// Check if string has changed (from KREAD), if so, update.
		if (strcmp(vdev_raidz_string,
		    KSTAT_NAMED_STR_PTR(&ks->zfs_vdev_raidz_impl)) != 0)
			vdev_raidz_impl_set(
			    KSTAT_NAMED_STR_PTR(&ks->zfs_vdev_raidz_impl));

		if (strcmp(icp_gcm_string,
		    KSTAT_NAMED_STR_PTR(&ks->icp_gcm_impl)) != 0)
			gcm_impl_set(KSTAT_NAMED_STR_PTR(&ks->icp_gcm_impl));

		if (strcmp(icp_aes_string,
		    KSTAT_NAMED_STR_PTR(&ks->icp_aes_impl)) != 0)
			aes_impl_set(KSTAT_NAMED_STR_PTR(&ks->icp_aes_impl));

		if (strcmp(zfs_fletcher_4_string,
		    KSTAT_NAMED_STR_PTR(&ks->zfs_fletcher_4_impl)) != 0)
			fletcher_4_impl_set(
			    KSTAT_NAMED_STR_PTR(&ks->zfs_fletcher_4_impl));

		zfs_expire_snapshot =
		    ks->zfs_expire_snapshot.value.ui64;
		zfs_admin_snapshot =
		    ks->zfs_admin_snapshot.value.ui64;
		zfs_auto_snapshot =
		    ks->zfs_auto_snapshot.value.ui64;

		zfs_disable_spotlight = ks->zfs_disable_spotlight.value.ui64;
		zfs_disable_trashes = ks->zfs_disable_trashes.value.ui64;
		zfs_dbgmsg_enable = ks->zfs_dbgmsg_enable.value.ui64;
		zfs_dbgmsg_maxsize = ks->zfs_dbgmsg_maxsize.value.ui64;

	} else {

		/* kstat READ */
		ks->spa_version.value.ui64 = SPA_VERSION;
		ks->zpl_version.value.ui64 = ZPL_VERSION;

		/* Darwin */
		ks->darwin_active_vnodes.value.ui64 = vnop_num_vnodes;
		ks->darwin_reclaim_nodes.value.ui64 = vnop_num_reclaims;
		ks->darwin_ignore_negatives.value.ui64 =
		    zfs_vnop_ignore_negatives;
		ks->darwin_ignore_positives.value.ui64 =
		    zfs_vnop_ignore_positives;
		ks->darwin_create_negatives.value.ui64 =
		    zfs_vnop_create_negatives;
		ks->darwin_force_formd_normalized.value.ui64 =
		    zfs_vnop_force_formd_normalized_output;
		ks->darwin_skip_unlinked_drain.value.ui64 =
		    zfs_vnop_skip_unlinked_drain;
		ks->darwin_use_system_sync.value.ui64 = zfs_vfs_sync_paranoia;

		vdev_raidz_impl_get(vdev_raidz_string,
		    sizeof (vdev_raidz_string));
		kstat_named_setstr(&ks->zfs_vdev_raidz_impl, vdev_raidz_string);

		gcm_impl_get(icp_gcm_string, sizeof (icp_gcm_string));
		kstat_named_setstr(&ks->icp_gcm_impl, icp_gcm_string);

		aes_impl_get(icp_aes_string, sizeof (icp_aes_string));
		kstat_named_setstr(&ks->icp_aes_impl, icp_aes_string);

		fletcher_4_get(zfs_fletcher_4_string,
		    sizeof (zfs_fletcher_4_string));
		kstat_named_setstr(&ks->zfs_fletcher_4_impl,
		    zfs_fletcher_4_string);

		ks->zfs_expire_snapshot.value.ui64 = zfs_expire_snapshot;
		ks->zfs_admin_snapshot.value.ui64 = zfs_admin_snapshot;
		ks->zfs_auto_snapshot.value.ui64 = zfs_auto_snapshot;

		ks->zfs_disable_spotlight.value.ui64 = zfs_disable_spotlight;

		ks->zfs_disable_trashes.value.ui64 = zfs_disable_trashes;
		ks->zfs_dbgmsg_enable.value.ui64 = zfs_dbgmsg_enable;
		ks->zfs_dbgmsg_maxsize.value.ui64 = zfs_dbgmsg_maxsize;

	}

	return (0);
}



int
kstat_osx_init(void)
{
	osx_kstat_ksp = kstat_create("zfs", 0, "tunable", "darwin",
	    KSTAT_TYPE_NAMED, sizeof (osx_kstat) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL|KSTAT_FLAG_WRITABLE);

	if (osx_kstat_ksp != NULL) {
		osx_kstat_ksp->ks_data = &osx_kstat;
		osx_kstat_ksp->ks_update = osx_kstat_update;
		kstat_install(osx_kstat_ksp);
	}

	return (0);
}

void
kstat_osx_fini(void)
{
	if (osx_kstat_ksp != NULL) {
		kstat_delete(osx_kstat_ksp);
		osx_kstat_ksp = NULL;
	}
}
