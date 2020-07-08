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
 * Jorgen Lundman <lundman@lundman.net>
 */

#ifndef _SPL_KERNEL_MAP_H
#define	_SPL_KERNEL_MAP_H

/* Skip if requested; see spl-osx.c */
#ifndef KERNEL_MAP_NO_LOAD

struct fileproc;
struct decmpfs_cnode;

/* List of pointers to be filled in, to the real locations */
extern volatile unsigned int *REAL_vm_page_free_wanted;
extern volatile unsigned int *REAL_vm_page_free_count;
extern volatile unsigned int *REAL_vm_page_speculative_count;
extern volatile unsigned int *REAL_vm_page_free_min;
extern int *REAL_system_inshutdown;
extern char *REAL_hostname;
#ifdef DEBUG
extern OSKextLoadedKextSummaryHeader *gLoadedKextSummaries;
extern kernel_mach_header_t *REAL__mh_execute_header;
#endif

/* Functions */
extern struct vnode *(*REAL_rootvnode);
extern vfs_context_t (*REAL_vfs_context_kernel)(void);
extern int (*REAL_vnode_iocount)(struct vnode *vp);
extern void (*REAL_cache_purgevfs)(mount_t mp);
extern int (*REAL_VFS_ROOT)(mount_t mp, struct vnode **vpp, vfs_context_t ctx);
extern errno_t (*REAL_VNOP_LOOKUP)(struct vnode *dvp, struct vnode **vpp,
		struct componentname *cnp, vfs_context_t ctx);
extern int (*REAL_build_path)(struct vnode *vp, char *buff, int buflen, int *outlen,
		int flags, vfs_context_t ctx);
extern addr64_t  (*REAL_kvtophys)(vm_offset_t va);

extern kern_return_t (*REAL_kernel_memory_allocate)(vm_map_t map, void **addrp,
		vm_size_t size, vm_offset_t mask, int flags, int tag);
extern int (*REAL_kauth_cred_getgroups)(kauth_cred_t _cred, gid_t *_groups,
    int *_groupcount);
extern int (*REAL_vnode_iocount)(struct vnode *vp);

//extern i386_cpu_info_t *(*REAL_cpuid_info)(void);

extern struct decmpfs_cnode *(*REAL_decmpfs_cnode_alloc)(void);
extern void (*REAL_decmpfs_cnode_free)(struct decmpfs_cnode *dp);
extern void (*REAL_decmpfs_cnode_init)(struct decmpfs_cnode *cp);
extern void (*REAL_decmpfs_cnode_destroy)(struct decmpfs_cnode *cp);
extern int (*REAL_decmpfs_decompress_file)(struct vnode *vp, struct decmpfs_cnode *cp, off_t toSize, int truncate_okay, int skiplock);
extern int (*REAL_decmpfs_file_is_compressed)(struct vnode *vp, struct decmpfs_cnode *cp);

#ifdef __arm64__
extern int (*REAL_setjmp)(void *e);
extern void (*longjmp)(void *e, int val);
#endif

#if 0
extern int (*REAL_fp_drop)(struct proc *p, int fd, struct fileproc *fp, int locked);
extern int (*REAL_fp_drop_written)(struct proc *p, int fd, struct fileproc *fp, int locked);
extern int (*REAL_fp_lookup)(struct proc *p, int fd, struct fileproc **resultfp, int locked);
extern int (*REAL_fo_read)(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx);
extern int (*REAL_fo_write)(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx);
#endif

/* Make consumers use the pointers */


/* List of pointers to be filled in, to the real locations */
#define	vm_page_free_wanted (*REAL_vm_page_free_wanted)
#define	vm_page_free_count (*REAL_vm_page_free_count)
#define	vm_page_free_min (*REAL_vm_page_free_min)
#define	vm_page_speculative_count (*REAL_vm_page_speculative_count)
#define	system_inshutdown (*REAL_system_inshutdown)
#define	hostname (*REAL_hostname)
#ifdef DEBUG
#define	gLoadedKextSummaries (*REAL_gLoadedKextSummaries)
#define	_mh_execute_header (*REAL__mh_execute_header)
#endif

/* Functions */
#define	rootvnode (*REAL_rootvnode)
#define	vfs_context_kernel (*REAL_vfs_context_kernel)
#define	vnode_iocount (*REAL_vnode_iocount)
#define	cache_purgevfs (*REAL_cache_purgevfs)
//#define	VFS_ROOT (*REAL_VFS_ROOT)
#define	VNOP_LOOKUP (*REAL_VNOP_LOOKUP)
//#define	build_path (*REAL_build_path)
#define	kvtophys  (*REAL_kvtophys)
#define	kernel_memory_allocate (*REAL_kernel_memory_allocate)
#define	kauth_cred_getgroups (*REAL_kauth_cred_getgroups)

#define	cpuid_info (*REAL_cpuid_info)
#if 0
#define	decmpfs_cnode_alloc (*REAL_decmpfs_cnode_alloc)
#define	decmpfs_cnode_free (*REAL_decmpfs_cnode_free)
#define	decmpfs_cnode_init (*REAL_decmpfs_cnode_init)
#define	decmpfs_cnode_destroy (*REAL_decmpfs_cnode_destroy)
#define	decmpfs_decompress_file (*REAL_decmpfs_decompress_file)
#define	decmpfs_file_is_compressed (*REAL_decmpfs_file_is_compressed)
#endif

#if 0
#define	fp_drop (*REAL_fp_drop)
#define	fp_drop_written (*REAL_fp_drop_written)
#define	fp_lookup (*REAL_fp_lookup)
#define	fo_read (*REAL_fo_read)
#define	fo_write (*REAL_fo_write)
#endif

#endif // NO_MAP
#endif
