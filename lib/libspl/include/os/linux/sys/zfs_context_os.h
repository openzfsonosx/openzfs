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

#ifndef ZFS_CONTEXT_OS_H
#define	ZFS_CONTEXT_OS_H


#define	zuio_segflg(U)		(U)->uio_segflg
#define	zuio_offset(U)		(U)->uio_loffset
#define	zuio_iovcnt(U)		(U)->uio_iovcnt
#define	zuio_iovlen(U, I)	(U)->uio_iov[(I)].iov_len
#define	zuio_iovbase(U, I)	(U)->uio_iov[(I)].iov_base
#define zuio_update(U, N)	\
	do {					\
	(U)->uio_resid -= (N);	\
	(U)->uio_loffset += (N);\
	while(0)
#define zuio_nonemptyindex(U, O, I)						\
	for ((I) = 0; (I) < (U)->uio_iovcnt &&				\
	    (O) >= (U)->uio_iov[(I)].iov_len;				\
	    (O) -= (U)->uio_iov[(I)++].iov_len)				\
		;
#define	zuio_iov(U, I, B, S)					\
	(B) = zuio_iovbase((U), (I));				\
	(S) = zuio_iovlen((U), (I))

#endif
