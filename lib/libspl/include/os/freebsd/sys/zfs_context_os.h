/*
 * Copyright (c) 2020 iXsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef ZFS_CONTEXT_OS_H_
#define	ZFS_CONTEXT_OS_H_

#if BYTE_ORDER != BIG_ENDIAN
#undef _BIG_ENDIAN
#endif

#define	ZFS_EXPORTS_PATH	"/etc/zfs/exports"

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
