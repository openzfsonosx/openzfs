#ifndef _SPL_KMEM_CACHE_H
#define	_SPL_KMEM_CACHE_H

#include <sys/taskq.h>

/* kmem move callback return values */
typedef enum kmem_cbrc {
	KMEM_CBRC_YES		= 0,	/* Object moved */
	KMEM_CBRC_NO		= 1,	/* Object not moved */
	KMEM_CBRC_LATER		= 2,	/* Object not moved, try again later */
	KMEM_CBRC_DONT_NEED	= 3,	/* Neither object is needed */
	KMEM_CBRC_DONT_KNOW	= 4,	/* Object unknown */
} kmem_cbrc_t;

extern void spl_kmem_cache_set_move(kmem_cache_t *,
    kmem_cbrc_t (*)(void *, void *, size_t, void *));

#define	kmem_cache_set_move(skc, move)	spl_kmem_cache_set_move(skc, move)

#endif
