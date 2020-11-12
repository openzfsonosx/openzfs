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
 * Copyright 2010 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */


#ifndef _SPL_UIO_H
#define	_SPL_UIO_H


// OSX defines "uio_t" as "struct uio *"
// ZFS defines "uio_t" as "struct uio"
#undef uio_t
#include_next <sys/uio.h>
#define	uio_t struct uio

#include <sys/types.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct iovec iovec_t;

typedef enum uio_seg uio_seg_t;
typedef enum uio_rw uio_rw_t;

typedef struct aio_req {
	uio_t		*aio_uio;
	void		*aio_private;
} aio_req_t;

typedef enum xuio_type {
	UIOTYPE_ASYNCIO,
	UIOTYPE_ZEROCOPY,
} xuio_type_t;


#define	UIOA_IOV_MAX    16

typedef struct uioa_page_s {
	int	uioa_pfncnt;
	void	**uioa_ppp;
	caddr_t	uioa_base;
	size_t	uioa_len;
} uioa_page_t;

typedef struct xuio {
	uio_t *xu_uio;
	enum xuio_type xu_type;
	union {
		struct {
			uint32_t xu_a_state;
			ssize_t xu_a_mbytes;
			uioa_page_t *xu_a_lcur;
			void **xu_a_lppp;
			void *xu_a_hwst[4];
			uioa_page_t xu_a_locked[UIOA_IOV_MAX];
		} xu_aio;

		struct {
			int xu_zc_rw;
			void *xu_zc_priv;
		} xu_zc;
	} xu_ext;
} xuio_t;

#define	XUIO_XUZC_PRIV(xuio)	xuio->xu_ext.xu_zc.xu_zc_priv
#define	XUIO_XUZC_RW(xuio)	xuio->xu_ext.xu_zc.xu_zc_rw

#define	uio_segflg(U) \
	(uio_isuserspace((struct uio *)(U))?UIO_USERSPACE:UIO_SYSSPACE)
#define	uio_advance(U, N)	uio_update((struct uio *)(U), (N))

static inline uint64_t
uio_iovlen(const struct uio *u, unsigned int i)
{
	user_size_t iov_len;
	uio_getiov((struct uio *)u, i, NULL, &iov_len);
	return (iov_len);
}

static inline void *
uio_iovbase(const struct uio *u, unsigned int i)
{
	user_addr_t iov_base;
	uio_getiov((struct uio *)u, i, &iov_base, NULL);
	return ((void *)iov_base);
}

static inline void
uio_iov_at_index(uio_t *uio, unsigned int idx, void **base, uint64_t *len)
{
	(void) uio_getiov(uio, idx, (user_addr_t *)base, len);
}

static inline long long
uio_index_at_offset(struct uio *uio, long long off, unsigned int *vec_idx)
{
	uint64_t len;
	*vec_idx = 0;
	while (*vec_idx < uio_iovcnt(uio) && off >=
	    (len = uio_iovlen(uio, *vec_idx))) {
		off -= len;
		(*vec_idx)++;
	}
	return (off);
}

/*
 * same as uiomove() but doesn't modify uio structure.
 * return in cbytes how many bytes were copied.
 */
static inline int
uiocopy(const char *p, size_t n, enum uio_rw rw, struct uio *uio,
    size_t *cbytes)
{
	int result;
	struct uio *nuio = uio_duplicate(uio);
	unsigned long long x = uio_resid(uio);
	if (!nuio)
		return (ENOMEM);
	uio_setrw(nuio, rw);
	result = uiomove(p, n, nuio);
	*cbytes = x-uio_resid(nuio);
	uio_free(nuio);
	return (result);
}


// Apple's uiomove puts the uio_rw in uio_create
#define	uiomove(A, B, C, D)	uiomove((A), (B), (D))
#define	uioskip(A, B)		uio_update((A), (B))

extern int uio_prefaultpages(ssize_t, uio_t *);
#define	uio_fault_disable(uio, set)

#ifdef  __cplusplus
}
#endif
#endif /* SPL_UIO_H */
