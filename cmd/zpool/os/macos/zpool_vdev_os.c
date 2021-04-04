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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <libnvpair.h>
#include <libzutil.h>
#include <limits.h>
#include <sys/spa.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "zpool_util.h"
#include <sys/zfs_context.h>

#include <sys/efi_partition.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <sys/mntent.h>
#include <uuid/uuid.h>
#include <libdiskmgt.h>

boolean_t
check_sector_size_database(char *path, int *sector_size)
{
	return (B_FALSE);
}

static void
check_error(int err)
{
	/*
	 * ENXIO/ENODEV is a valid error message if the device doesn't live in
	 * /dev/dsk.  Don't bother printing an error message in this case.
	 */
	if (err == ENXIO || err == ENODEV)
		return;

	(void) fprintf(stderr, gettext("warning: device in use checking "
	    "failed: %s\n"), strerror(err));
}

static int
check_slice(const char *path, int force, boolean_t isspare)
{
	char *msg;
	int error = 0;
	dm_who_type_t who;

	if (force)
		who = DM_WHO_ZPOOL_FORCE;
	else if (isspare)
		who = DM_WHO_ZPOOL_SPARE;
	else
		who = DM_WHO_ZPOOL;

	if (dm_inuse((char *)path, &msg, who, &error) || error) {
		if (error != 0) {
			check_error(error);
			return (0);
		} else {
			vdev_error("%s", msg);
			free(msg);
			return (-1);
		}
	}
	return (0);
}

static int
check_disk(const char *path, int force,
    boolean_t isspare, boolean_t iswholedisk)
{
	struct dk_gpt *vtoc;
	char slice_path[MAXPATHLEN];
	int err = 0;
	int slice_err = 0;
	int fd, i;

	if (!iswholedisk)
		return (check_slice(path, force, isspare));

	if ((fd = open(path, O_RDONLY|O_DIRECT)) < 0) {
		check_error(errno);
		return (-1);
	}
	/*
	 * Expected to fail for non-EFI labled disks.  Just check the device
	 * as given and do not attempt to detect and scan partitions.
	 */
	err = efi_alloc_and_read(fd, &vtoc);
	if (err) {
		(void) close(fd);
		return (check_slice(path, force, isspare));
	}

	/*
	 * The primary efi partition label is damaged however the secondary
	 * label at the end of the device is intact.  Rather than use this
	 * label we should play it safe and treat this as a non efi device.
	 */
	if (vtoc->efi_flags & EFI_GPT_PRIMARY_CORRUPT) {
		efi_free(vtoc);
		(void) close(fd);

		if (force) {
			/* Partitions will no be created using the backup */
			return (0);
		} else {
			vdev_error(gettext("%s contains a corrupt primary "
			    "EFI label.\n"), path);
			return (-1);
		}
	}
	for (i = 0; i < vtoc->efi_nparts; i++) {
		if (vtoc->efi_parts[i].p_tag == V_UNASSIGNED ||
		    uuid_is_null((uchar_t *)&vtoc->efi_parts[i].p_guid))
			continue;
		(void) snprintf(slice_path, sizeof (slice_path),
		    "%ss%d", path, i+1);
		slice_err = check_slice(slice_path, force, isspare);

		// Latch the first error that occurs
		if (err == 0)
			err = slice_err;
	}

	efi_free(vtoc);
	(void) close(fd);

	return (err);
}


int
check_device(const char *name, boolean_t force,
    boolean_t isspare, boolean_t iswholedisk)
{
	char path[MAXPATHLEN];
	int error;

	if (strncmp(name, _PATH_DEV, sizeof (_PATH_DEV) - 1) != 0)
		snprintf(path, sizeof (path), "%s%s", _PATH_DEV, name);
	else
		strlcpy(path, name, sizeof (path));

	error = check_disk(path, force, isspare, iswholedisk);
	if (error != 0)
		return (error);

	return (check_file(path, force, isspare));
}

int
check_file(const char *file, boolean_t force, boolean_t isspare)
{
	if (dm_in_swap_dir(file)) {
		vdev_error(gettext(
		    "%s is located within the swapfile directory.\n"), file);
		return (-1);
	}

	return (check_file_generic(file, force, isspare));
}
