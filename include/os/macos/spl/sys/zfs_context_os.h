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
#ifndef _SPL_ZFS_CONTEXT_OS_H
#define _SPL_ZFS_CONTEXT_OS_H

#define MSEC_TO_TICK(msec)		((msec) / (MILLISEC / hz))

#define	noinline		__attribute__((noinline))

/* really? */
#define	kpreempt_disable()		((void)0)
#define	kpreempt_enable()		((void)0)

/* Make sure kmem and vmem are already included */
#include <sys/seg_kmem.h>
#include <sys/kmem.h>

/* Since Linux code uses vmem_free() and we already have one: */
#define vmem_free(A, B)			zfs_kmem_free((A), (B))
#define vmem_alloc(A, B)		zfs_kmem_alloc((A), (B))
#define vmem_zalloc(A, B)		zfs_kmem_zalloc((A), (B))

typedef	int	fstrans_cookie_t;
#define	spl_fstrans_mark()		(0)
#define	spl_fstrans_unmark(x)	(x = 0)

#define	zuio_offset(U)		uio_offset((U))
#define	zuio_update(U, N)	uio_update((U), (N))

#endif

