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
 * Copyright 2015 Nexenta Systems, Inc.  All rights reserved.
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2014, 2019 by Delphix. All rights reserved.
 * Copyright 2016 Igor Kozhukhov <ikozhukhov@gmail.com>
 * Copyright 2017 RackTop Systems.
 * Copyright (c) 2018 Datto Inc.
 * Copyright 2018 OmniOS Community Edition (OmniOSce) Association.
 */

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <zone.h>
#include <sys/mntent.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/dsl_crypt.h>
#include <os/macos/zfs/sys/zfs_mount.h>
#include <libzfs.h>

#include "libzfs_impl.h"
#include <thread_pool.h>
#include <sys/sysctl.h>

/*
 * zfs_init_libshare(zhandle, service)
 *
 * Initialize the libshare API if it hasn't already been initialized.
 * In all cases it returns 0 if it succeeded and an error if not. The
 * service value is which part(s) of the API to initialize and is a
 * direct map to the libshare sa_init(service) interface.
 */
int
zfs_init_libshare(libzfs_handle_t *zhandle, int service)
{
	int ret = SA_OK;

	if (ret == SA_OK && zhandle->libzfs_shareflags & ZFSSHARE_MISS) {
		/*
		 * We had a cache miss. Most likely it is a new ZFS
		 * dataset that was just created. We want to make sure
		 * so check timestamps to see if a different process
		 * has updated any of the configuration. If there was
		 * some non-ZFS change, we need to re-initialize the
		 * internal cache.
		 */
		zhandle->libzfs_shareflags &= ~ZFSSHARE_MISS;
		if (sa_needs_refresh(zhandle->libzfs_sharehdl)) {
			zfs_uninit_libshare(zhandle);
			zhandle->libzfs_sharehdl = sa_init(service);
		}
	}

	if (ret == SA_OK && zhandle && zhandle->libzfs_sharehdl == NULL)
		zhandle->libzfs_sharehdl = sa_init(service);

	if (ret == SA_OK && zhandle->libzfs_sharehdl == NULL)
		ret = SA_NO_MEMORY;
	return (ret);
}


/*
 * Share the given filesystem according to the options in the specified
 * protocol specific properties (sharenfs, sharesmb).  We rely
 * on "libshare" to do the dirty work for us.
 */
int
zfs_share_proto(zfs_handle_t *zhp, zfs_share_proto_t *proto)
{
	char mountpoint[ZFS_MAXPROPLEN];
	char shareopts[ZFS_MAXPROPLEN];
	char sourcestr[ZFS_MAXPROPLEN];
	libzfs_handle_t *hdl = zhp->zfs_hdl;
	sa_share_t share;
	zfs_share_proto_t *curr_proto;
	zprop_source_t sourcetype;
	int err, ret;

	if (!zfs_is_mountable(zhp, mountpoint, sizeof (mountpoint), NULL, 0))
		return (0);

	for (curr_proto = proto; *curr_proto != PROTO_END; curr_proto++) {
		/*
		 * Return success if there are no share options.
		 */
		if (zfs_prop_get(zhp, proto_table[*curr_proto].p_prop,
		    shareopts, sizeof (shareopts), &sourcetype, sourcestr,
		    ZFS_MAXPROPLEN, B_FALSE) != 0 ||
		    strcmp(shareopts, "off") == 0)
			continue;

		ret = zfs_init_libshare(hdl, SA_INIT_SHARE_API);
		if (ret != SA_OK) {
			(void) zfs_error_fmt(hdl, EZFS_SHARENFSFAILED,
			    dgettext(TEXT_DOMAIN, "cannot share '%s': %s"),
			    zfs_get_name(zhp), sa_errorstr(ret));
			return (-1);
		}

		/*
		 * If the 'zoned' property is set, then zfs_is_mountable()
		 * will have already bailed out if we are in the global zone.
		 * But local zones cannot be NFS servers, so we ignore it for
		 * local zones as well.
		 */
		if (zfs_prop_get_int(zhp, ZFS_PROP_ZONED))
			continue;

		share = sa_find_share(hdl->libzfs_sharehdl, mountpoint);
		if (share == NULL) {
			/*
			 * This may be a new file system that was just
			 * created so isn't in the internal cache
			 * (second time through). Rather than
			 * reloading the entire configuration, we can
			 * assume ZFS has done the checking and it is
			 * safe to add this to the internal
			 * configuration.
			 */
			if (sa_zfs_process_share(hdl->libzfs_sharehdl,
			    NULL, NULL, mountpoint,
			    proto_table[*curr_proto].p_name, sourcetype,
			    shareopts, sourcestr, zhp->zfs_name) != SA_OK) {
				(void) zfs_error_fmt(hdl,
				    proto_table[*curr_proto].p_share_err,
				    dgettext(TEXT_DOMAIN, "cannot share '%s'"),
				    zfs_get_name(zhp));
				return (-1);
			}
			hdl->libzfs_shareflags |= ZFSSHARE_MISS;
			share = sa_find_share(hdl->libzfs_sharehdl,
			    mountpoint);
		}
		if (share != NULL) {
			err = sa_enable_share(share,
			    proto_table[*curr_proto].p_name);
			if (err != SA_OK) {
				(void) zfs_error_fmt(hdl,
				    proto_table[*curr_proto].p_share_err,
				    dgettext(TEXT_DOMAIN, "cannot share '%s'"),
				    zfs_get_name(zhp));
				return (-1);
			}
		} else {
			(void) zfs_error_fmt(hdl,
			    proto_table[*curr_proto].p_share_err,
			    dgettext(TEXT_DOMAIN, "cannot share '%s'"),
			    zfs_get_name(zhp));
			return (-1);
		}

	}
	return (0);
}

/*
 * Unshare a filesystem by mountpoint.
 */
int
unshare_one(libzfs_handle_t *hdl, const char *name, const char *mountpoint,
    zfs_share_proto_t proto)
{
	sa_share_t share;
	int err;
	char *mntpt;

	/*
	 * Mountpoint could get trashed if libshare calls getmntany
	 * which it does during API initialization, so strdup the
	 * value.
	 */
	mntpt = zfs_strdup(hdl, mountpoint);

	/* make sure libshare initialized */
	if ((err = zfs_init_libshare(hdl, SA_INIT_SHARE_API)) != SA_OK) {
		free(mntpt);	/* don't need the copy anymore */
		return (zfs_error_fmt(hdl, proto_table[proto].p_unshare_err,
		    dgettext(TEXT_DOMAIN, "cannot unshare '%s': %s"),
		    name, sa_errorstr(err)));
	}

	share = sa_find_share(hdl->libzfs_sharehdl, mntpt);
	free(mntpt);	/* don't need the copy anymore */

	if (share != NULL) {
		err = sa_disable_share(share, proto_table[proto].p_name);
		if (err != SA_OK) {
			return (zfs_error_fmt(hdl,
			    proto_table[proto].p_unshare_err,
			    dgettext(TEXT_DOMAIN, "cannot unshare '%s': %s"),
			    name, sa_errorstr(err)));
		}
	} else {
		return (zfs_error_fmt(hdl, proto_table[proto].p_unshare_err,
		    dgettext(TEXT_DOMAIN, "cannot unshare '%s': not found"),
		    name));
	}
	return (0);
}

/*
 * Search the sharetab for the given mountpoint and protocol, returning
 * a zfs_share_type_t value.
 */
zfs_share_type_t
is_shared_impl(libzfs_handle_t *hdl, const char *mountpoint,
    zfs_share_proto_t proto)
{
	char buf[MAXPATHLEN], *tab;
	char *ptr;

	if (hdl->libzfs_sharetab == NULL)
		return (SHARED_NOT_SHARED);

	/* Reopen ZFS_SHARETAB to prevent reading stale data from open file */
	if (freopen(ZFS_SHARETAB, "r", hdl->libzfs_sharetab) == NULL)
		return (SHARED_NOT_SHARED);

	(void) fseek(hdl->libzfs_sharetab, 0, SEEK_SET);

	while (fgets(buf, sizeof (buf), hdl->libzfs_sharetab) != NULL) {

		/* the mountpoint is the first entry on each line */
		if ((tab = strchr(buf, '\t')) == NULL)
			continue;

		*tab = '\0';
		if (strcmp(buf, mountpoint) == 0) {
			/*
			 * the protocol field is the third field
			 * skip over second field
			 */
			ptr = ++tab;
			if ((tab = strchr(ptr, '\t')) == NULL)
				continue;
			ptr = ++tab;
			if ((tab = strchr(ptr, '\t')) == NULL)
				continue;
			*tab = '\0';
			if (strcmp(ptr,
			    proto_table[proto].p_name) == 0) {
				switch (proto) {
				case PROTO_NFS:
					return (SHARED_NFS);
				case PROTO_SMB:
					return (SHARED_SMB);
				default:
					return (0);
				}
			}
		}
	}

	return (SHARED_NOT_SHARED);
}

/*
  if (zmount(zhp, zfs_get_name(zhp), mountpoint, MS_OPTIONSTR | flags,
  MNTTYPE_ZFS, NULL, 0, mntopts, sizeof (mntopts)) != 0) {
 */
int
do_mount(zfs_handle_t *zhp, const char *dir, char *optptr, int mflag)
{
/*
int
	zmount(zfs_handle_t *zhp, const char *spec, const char *dir, int mflag,
        char *fstype, char *dataptr, int datalen, char *optptr, int optlen)
*/
	int rv;
	const char *spec = zfs_get_name(zhp);
	const char *fstype = MNTTYPE_ZFS;
	struct zfs_mount_args mnt_args;
	char *rpath = NULL;
	zfs_cmd_t zc = { "\0" };
	int devdisk = ZFS_DEVDISK_POOLONLY;
	int ispool = 0;  // the pool dataset, that is
	int optlen = 0;

	assert(spec != NULL);
	assert(dir != NULL);
	assert(fstype != NULL);
	assert(mflag >= 0);
	assert(strcmp(fstype, MNTTYPE_ZFS) == 0);
	assert(dataptr == NULL);
	assert(datalen == 0);
	assert(optptr != NULL);
	assert(optlen > 0);

	if (optptr != NULL)
		optlen = strlen(optptr);

	/*
	 * Figure out if we want this mount as a /dev/diskX mount, if so
	 * ask kernel to create one for us, then use it to mount.
	 */

	// Use dataset name by default
	mnt_args.fspec = spec;

	/* Lookup the dataset property devdisk, and depending on its
	 * setting, we need to create a /dev/diskX for the mount
	 */
	if (zhp) {

		devdisk = zfs_prop_get_int(zhp, ZFS_PROP_DEVDISK);

		if (zhp && zhp->zpool_hdl &&
			!strcmp(zpool_get_name(zhp->zpool_hdl),
				zfs_get_name(zhp)))
			ispool = 1;

		if ((devdisk == ZFS_DEVDISK_ON) ||
			((devdisk == ZFS_DEVDISK_POOLONLY) &&
				ispool)) {
			(void)strlcpy(zc.zc_name, zhp->zfs_name, sizeof(zc.zc_name));
			zc.zc_value[0] = 0;

			rv = zfs_ioctl(zhp->zfs_hdl, ZFS_IOC_PROXY_DATASET, &zc);

#ifdef DEBUG
			if (rv)
				fprintf(stderr, "proxy dataset returns %d '%s'\n",
					rv, zc.zc_value);
#endif

			// Mount using /dev/diskX, use temporary buffer to give it full
			// name
			if (rv == 0) {
				snprintf(zc.zc_name, sizeof(zc.zc_name),
					"/dev/%s", zc.zc_value);
				mnt_args.fspec = zc.zc_name;
			}
		}
	}

	mnt_args.mflag = mflag;
	mnt_args.optptr = optptr;
	mnt_args.optlen = optlen;
	mnt_args.struct_size = sizeof(mnt_args);

	/* There is a bug in XNU where /var/tmp is resolved as
	 * "private/var/tmp" without the leading "/", and both mount(2) and
	 * diskutil mount avoid this by calling realpath() first. So we will
	 * do the same.
	 */
	rpath = realpath(dir, NULL);

	dprintf("%s calling mount with fstype %s, %s %s, fspec %s, mflag %d,"
		" optptr %s, optlen %d, devdisk %d, ispool %d\n",
		__func__, fstype, (rpath ? "rpath" : "dir"),
		(rpath ? rpath : dir), mnt_args.fspec, mflag, optptr, optlen,
		devdisk, ispool);
	rv = mount(fstype, rpath ? rpath : dir, 0, &mnt_args);

	if (rpath) free(rpath);

	return rv;
}

int
do_unmount_impl(const char *mntpt, int flags)
{
	char force_opt[] = "force";
	char *argv[7] = {
		"/usr/sbin/diskutil",
		"unmount",
		NULL, NULL, NULL, NULL };
	int rc, count = 2;

	if (flags & MS_FORCE) {
		argv[count] = force_opt;
		count++;
	}

	argv[count] = (char *)mntpt;
	rc = libzfs_run_process(argv[0], argv, STDOUT_VERBOSE|STDERR_VERBOSE);

	/* There is a bug, where we can not unmount, with the error
	 * already unmounted, even though it wasn't. But it is easy
	 * to work around by calling 'umount'. Until a real fix is done...
	 * re-test this: 202004/lundman
	 */
	if (rc != 0) {
		char *argv[7] = {
			"/sbin/umount",
			NULL, NULL, NULL, NULL };
		int rc, count = 1;

		fprintf(stderr, "Fallback umount called\r\n");
		if (flags & MS_FORCE) {
			argv[count] = "-f";
			count++;
		}
		argv[count] = (char *)mntpt;
		rc = libzfs_run_process(argv[0], argv, STDOUT_VERBOSE|STDERR_VERBOSE);
	}

	return (rc ? EINVAL : 0);
}


void unmount_snapshots(libzfs_handle_t *hdl, const char *mntpt, int flags);

int
do_unmount(libzfs_handle_t *hdl, const char *mntpt, int flags)
{
	/*
	 * On OSX, the kernel can not unmount all snapshots for us, as XNU
	 * rejects the unmount before it reaches ZFS. But we can easily handle
	 * unmounting snapshots from userland.
	 */
	unmount_snapshots(hdl, mntpt, flags);

	return do_unmount_impl(mntpt, flags);
}

/*
 * Given "/Volumes/BOOM" look for any lower mounts with ".zfs/snapshot/"
 * in them - issue unmount.
 */
void unmount_snapshots(libzfs_handle_t *hdl, const char *mntpt, int flags)
{
	struct mnttab entry;
	int len = strlen(mntpt);

	while (getmntent(hdl->libzfs_mnttab, &entry) == 0) {
		/* Starts with our mountpoint ? */
		if (strncmp(mntpt, entry.mnt_mountp, len) == 0) {
			/* The next part is "/.zfs/snapshot/" ? */
			if (strncmp("/.zfs/snapshot/", &entry.mnt_mountp[len],
					15) == 0) {
				/* Unmount it */
				do_unmount_impl(entry.mnt_mountp, MS_FORCE);
			}
		}
	}
}

int
zfs_mount_delegation_check(void)
{
	return ((geteuid() != 0) ? EACCES : 0);
}

static char *
zfs_snapshot_mountpoint(zfs_handle_t *zhp)
{
	char *dataset_name, *snapshot_mountpoint, *parent_mountpoint;
	libzfs_handle_t *hdl = zhp->zfs_hdl;
	zfs_handle_t *parent;
	char *r;

	dataset_name = zfs_strdup(hdl, zhp->zfs_name);
	if (dataset_name == NULL) {
		(void) fprintf(stderr, gettext("not enough memory"));
		return (NULL);
	}

	r = strrchr(dataset_name, '@');

	if (r == NULL) {
		(void) fprintf(stderr, gettext("snapshot '%s' "
		    "has no '@'\n"), zhp->zfs_name);
		free(dataset_name);
		return (NULL);
	}

	r[0] = 0;

	/* Open the dataset */
	if ((parent = zfs_open(hdl, dataset_name,
		    ZFS_TYPE_FILESYSTEM)) == NULL) {
		(void) fprintf(stderr, gettext("unable to open parent dataset '%s'\n"
		    ), dataset_name);
		free(dataset_name);
		return (NULL);
	}

	if (!zfs_is_mounted(parent, &parent_mountpoint)) {
		(void) fprintf(stderr, gettext("parent dataset '%s' must be mounted\n"
		    ), dataset_name);
		free(dataset_name);
		zfs_close(parent);
		return (NULL);
	}

	zfs_close(parent);

	snapshot_mountpoint =
		zfs_asprintf(hdl, "%s/.zfs/snapshot/%s/",
			parent_mountpoint, &r[1]);

	free(dataset_name);
	free(parent_mountpoint);

	return (snapshot_mountpoint);
}

/*
 * Mount a snapshot; called from "zfs mount dataset@snapshot".
 * Given "dataset@snapshot" construct mountpoint path of the
 * style "/mountpoint/dataset/.zfs/snapshot/$name/". Ensure
 * parent "dataset" is mounted, then issue mount for snapshot.
 */
int
zfs_snapshot_mount(zfs_handle_t *zhp, const char *options,
	int flags)
{
	int ret = 0;
	char *mountpoint;

	/*
	 * The automounting will kick in, and zed mounts it - so
	 * we temporarily disable it
	 */
	uint64_t automount = 0;
	uint64_t saved_automount = 0;
	size_t len = sizeof(automount);
	size_t slen = sizeof(saved_automount);

	/* Remember what the user has it set to */
	sysctlbyname("kstat.zfs.darwin.tunable.zfs_auto_snapshot",
	    &saved_automount, &slen, NULL, 0);

	/* Disable automounting */
	sysctlbyname("kstat.zfs.darwin.tunable.zfs_auto_snapshot",
	    NULL, NULL, &automount, len);

	if (zfs_is_mounted(zhp, NULL)) {
		return (EBUSY);
	}

	mountpoint = zfs_snapshot_mountpoint(zhp);
	if (mountpoint == NULL)
		return (EINVAL);

	ret = zfs_mount_at(zhp, options, MS_RDONLY | flags,
		mountpoint);

	/* If zed is running, it can mount it before us */
	if (ret == -1 && errno == EINVAL)
		ret = 0;

	if (ret == 0) {
		(void) fprintf(stderr, gettext("ZFS: snapshot mountpoint '%s'\n"),
			mountpoint);
	}

	free(mountpoint);

	/* Restore automount setting */
	sysctlbyname("kstat.zfs.darwin.tunable.zfs_auto_snapshot",
	    NULL, NULL, &saved_automount, len);

	return ret;
}

int
zfs_snapshot_unmount(zfs_handle_t *zhp, int flags)
{
	int ret = 0;
	char *mountpoint;

	if (!zfs_is_mounted(zhp, NULL)) {
		return (ENOENT);
	}

	mountpoint = zfs_snapshot_mountpoint(zhp);
	if (mountpoint == NULL)
		return (EINVAL);

	ret = zfs_unmount(zhp, mountpoint, flags);

	free(mountpoint);

	return ret;
}
