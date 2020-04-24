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

#ifndef _SPL_FILE_H
#define _SPL_FILE_H

#define	FIGNORECASE			0x00080000
#define	FKIOCTL				0x80000000
#define	ED_CASE_CONFLICT	0x10

#include <sys/list.h>

struct spl_fileproc {
    void        *f_vnode;  // this points to the "fd" so we can look it up.
    list_node_t  f_next;   /* next zfsdev_state_t link */
    int          f_fd;
    uint64_t     f_offset;
    void        *f_proc;
    void        *f_fp;
    int          f_writes;
	minor_t      f_file; // Minor of the file
};

#define file_t struct spl_fileproc

void *getf(int fd);
void releasef(int fd);
struct vnode *getf_vnode(void *fp);

#endif /* SPL_FILE_H */
