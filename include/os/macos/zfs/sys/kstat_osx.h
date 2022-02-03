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
 * Copyright (c) 2014, 2016 Jorgen Lundman <lundman@lundman.net>
 */

#ifndef KSTAT_OSX_INCLUDED
#define	KSTAT_OSX_INCLUDED

typedef struct osx_kstat {
	kstat_named_t spa_version;
	kstat_named_t zpl_version;

	kstat_named_t darwin_active_vnodes;
	kstat_named_t darwin_debug;
	kstat_named_t darwin_reclaim_nodes;
	kstat_named_t darwin_ignore_negatives;
	kstat_named_t darwin_ignore_positives;
	kstat_named_t darwin_create_negatives;
	kstat_named_t darwin_force_formd_normalized;
	kstat_named_t darwin_skip_unlinked_drain;
	kstat_named_t darwin_use_system_sync;

	kstat_named_t zfs_vdev_raidz_impl;
	kstat_named_t icp_gcm_impl;
	kstat_named_t icp_aes_impl;
	kstat_named_t zfs_fletcher_4_impl;

	kstat_named_t zfs_expire_snapshot;
	kstat_named_t zfs_admin_snapshot;
	kstat_named_t zfs_auto_snapshot;

	kstat_named_t zfs_disable_spotlight;
	kstat_named_t zfs_disable_trashes;
	kstat_named_t zfs_dbgmsg_enable;
	kstat_named_t zfs_dbgmsg_maxsize;
} osx_kstat_t;

extern unsigned int zfs_vnop_ignore_negatives;
extern unsigned int zfs_vnop_ignore_positives;
extern unsigned int zfs_vnop_create_negatives;
extern unsigned int zfs_vnop_skip_unlinked_drain;
extern uint64_t zfs_vfs_sync_paranoia;
extern uint64_t vnop_num_vnodes;
extern uint64_t vnop_num_reclaims;
extern int zfs_vnop_force_formd_normalized_output;

extern int zfs_expire_snapshot;
extern int zfs_admin_snapshot;
extern int zfs_auto_snapshot;

extern unsigned int zfs_disable_spotlight;
extern unsigned int zfs_disable_trashes;
extern int zfs_dbgmsg_enable;
extern int zfs_dbgmsg_maxsize;

int kstat_osx_init(void);
void kstat_osx_fini(void);

int arc_kstat_update(kstat_t *ksp, int rw);
int arc_kstat_update_osx(kstat_t *ksp, int rw);

#endif
