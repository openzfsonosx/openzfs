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

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/pathname.h>
#include <sys/zfs_context.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_vfsops.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/dmu.h>
#include <sys/dsl_destroy.h>

#include <sys/dsl_deleg.h>
#include <sys/mount.h>
#include <sys/sunddi.h>

#include "zfs_namecheck.h"

static struct vnode *zfsctl_mknode_snapdir(struct vnode *);
//static struct vnode *zfsctl_mknode_shares(struct vnode *);
static struct vnode *zfsctl_snapshot_mknode(struct vnode *, uint64_t objset);

int (**zfsctl_ops_root_dvnodeops) (void *);
int (**zfsctl_ops_snapdir_dvnodeops) (void *);
int (**zfsctl_ops_snapshot_dvnodeops) (void *);

void
zfsctl_init(void)
{
}

void
zfsctl_fini(void)
{
}

boolean_t
zfsctl_is_node(struct vnode *vp)
{
    if (vnode_tag(vp) == VT_OTHER)
        return B_TRUE;
    return B_FALSE;
}

/*
 * Create the '.zfs' directory.  This directory is cached as part of the VFS
 * structure.  This results in a hold on the vfs_t.  The code in zfs_umount()
 * therefore checks against a vfs_count of 2 instead of 1.  This reference
 * is removed when the ctldir is destroyed in the unmount.
 */
void
zfsctl_create(zfsvfs_t *zfsvfs)
{
}


/*
 * Destroy the '.zfs' directory.  Only called when the filesystem is unmounted.
 * There might still be more references if we were force unmounted, but only
 * new zfs_inactive() calls can occur and they don't reference .zfs
 */
void
zfsctl_destroy(zfsvfs_t *zfsvfs)
{
}

/*
 * Given a root znode, retrieve the associated .zfs directory.
 * Add a hold to the vnode and return it.
 */
struct vnode *
zfsctl_root(znode_t *zp)
{
	return (zp->z_zfsvfs->z_ctldir);
}

int
zfsctl_root_lookup(struct vnode *dvp, char *nm, struct vnode **vpp,
	int flags, cred_t *cr,
	int *direntflags, pathname_t *realpnp)
{
	int err = -1;

	return (err);
}

void
zfsctl_snapshot_unmount(char *snapname, int flags __unused)
{
}

#if !defined (__OPTIMIZE__)
#pragma GCC diagnostic ignored "-Wframe-larger-than="
#endif
int
zfsctl_snapdir_lookup(struct vnop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		} */ *ap)
{
	int err = -1;

	return (err);
}

int
zfsctl_lookup_objset(vfs_t *vfsp, uint64_t objsetid, zfsvfs_t **zfsvfsp)
{
	int error = 0;

	return (error);
}

/*
 * Unmount any snapshots for the given filesystem.  This is called from
 * zfs_umount() - if we have a ctldir, then go through and unmount all the
 * snapshots.
 */
int
zfsctl_umount_snapshots(vfs_t *vfsp, int fflags, cred_t *cr)
{
	int error = 0;

	return (error);
}

