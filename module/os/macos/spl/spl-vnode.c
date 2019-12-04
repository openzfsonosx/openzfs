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
 * Copyright (C) 2008 MacZFS
 * Copyright (C) 2013 Jorgen Lundman <lundman@lundman.net>
 *
 */

#include <sys/vnode.h>
#include <sys/debug.h>
#include <sys/malloc.h>
#include <sys/list.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <IOKit/IOLib.h>

#include <sys/taskq.h>

int
vn_open(char *pnamep, enum uio_seg seg, int filemode, int createmode,
        struct vnode **vpp, enum create crwhy, mode_t umask)
{
    vfs_context_t vctx;
    int fmode;
    int error;

    fmode = filemode;
    if (crwhy)
        fmode |= O_CREAT;
    // TODO I think this should be 'fmode' instead of 'filemode'
    vctx = vfs_context_create((vfs_context_t)0);
    error = vnode_open(pnamep, filemode, createmode, 0, vpp, vctx);
    (void) vfs_context_rele(vctx);
    //printf("vn_open '%s' -> %d (vp %p)\n", pnamep, error, *vpp);
    return (error);
}

int
vn_openat(char *pnamep, enum uio_seg seg, int filemode, int createmode,
          struct vnode **vpp, enum create crwhy,
          mode_t umask, struct vnode *startvp)
{
    char *path;
    int pathlen = MAXPATHLEN;
    int error;

    path = (char *)kmem_zalloc(MAXPATHLEN, KM_SLEEP);

    error = vn_getpath(startvp, path, &pathlen);
    if (error == 0) {
        strlcat(path, pnamep, MAXPATHLEN);
        error = vn_open(path, seg, filemode, createmode, vpp, crwhy,
                        umask);
    }

    kmem_free(path, MAXPATHLEN);
    return (error);
}

extern errno_t vnode_rename(const char *, const char *, int, vfs_context_t);

errno_t
vnode_rename(const char *from, const char *to, int flags, vfs_context_t vctx)
{
    /*
     * We need proper KPI changes to be able to safely update
     * the zpool.cache file. For now, we return EPERM.
     */
    return (EPERM);
}

int
vn_rename(char *from, char *to, enum uio_seg seg)
{
    vfs_context_t vctx;
    int error;

    vctx = vfs_context_create((vfs_context_t)0);

    error = vnode_rename(from, to, 0, vctx);

    (void) vfs_context_rele(vctx);

    return (error);
}

extern errno_t vnode_remove(const char *, int, enum vtype, vfs_context_t);

errno_t
vnode_remove(const char *name, int flag, enum vtype type, vfs_context_t vctx)
{
    /*
     * Now that zed ZFS Event Daemon can handle the rename of zpool.cache
     * we will silence this limitation, and look in zed.d/config.sync.sh
     */
    /*
    IOLog("vnode_remove: \"%s\"\n", name);
    IOLog("zfs: vnode_remove not yet supported\n");
    */
    return (EPERM);
}


int
vn_remove(char *fnamep, enum uio_seg seg, enum rm dirflag)
{
    vfs_context_t vctx;
    enum vtype type;
    int error;

    type = dirflag == RMDIRECTORY ? VDIR : VREG;

    vctx = vfs_context_create((vfs_context_t)0);

    error = vnode_remove(fnamep, 0, type, vctx);

    (void) vfs_context_rele(vctx);

    return (error);
}

int zfs_vn_rdwr(enum uio_rw rw, struct vnode *vp, caddr_t base, ssize_t len,
                offset_t offset, enum uio_seg seg, int ioflag, rlim64_t ulimit,
                cred_t *cr, ssize_t *residp)
{
    uio_t *auio;
    int spacetype;
    int error=0;
    vfs_context_t vctx;

    spacetype = UIO_SEG_IS_USER_SPACE(seg) ? UIO_USERSPACE32 : UIO_SYSSPACE;

    vctx = vfs_context_create((vfs_context_t)0);
    auio = uio_create(1, 0, spacetype, rw);
    uio_reset(auio, offset, spacetype, rw);
    uio_addiov(auio, (uint64_t)(uintptr_t)base, len);

    if (rw == UIO_READ) {
        error = VNOP_READ(vp, auio, ioflag, vctx);
    } else {
        error = VNOP_WRITE(vp, auio, ioflag, vctx);
    }

    if (residp) {
        *residp = uio_resid(auio);
    } else {
        if (uio_resid(auio) && error == 0)
            error = EIO;
    }

    uio_free(auio);
    vfs_context_rele(vctx);

    return (error);
}


int
VOP_SPACE(struct vnode *vp, int cmd, struct flock *fl, int flags, offset_t off,
          cred_t *cr, void *ctx)
{
	int error = 0;
#ifdef F_PUNCHHOLE
	if (cmd == F_FREESP) {
		fpunchhole_t fpht;
		fpht.fp_flags = 0;
		fpht.fp_offset = fl->l_start;
		fpht.fp_length = fl->l_len;
		if (vnode_getwithref(vp) == 0) {
			error = VNOP_IOCTL(vp, F_PUNCHHOLE, (caddr_t)&fpht, 0, ctx);
			(void)vnode_put(vp);
		}
	}
#endif
    return (error);
}

int
VOP_CLOSE(struct vnode *vp, int flag, int count, offset_t off, void *cr, void *k)
{
    vfs_context_t vctx;
    int error;

    vctx = vfs_context_create((vfs_context_t)0);
    error = vnode_close(vp, flag & FWRITE, vctx);
    (void) vfs_context_rele(vctx);
    return (error);
}

int
VOP_FSYNC(struct vnode *vp, int flags, void* unused, void *uused2)
{
    vfs_context_t vctx;
    int error;

    vctx = vfs_context_create((vfs_context_t)0);
    error = VNOP_FSYNC(vp, (flags == FSYNC), vctx);
    (void) vfs_context_rele(vctx);
    return (error);
}

int VOP_GETATTR(struct vnode *vp, vattr_t *vap, int flags, void *x3, void *x4)
{
    vfs_context_t vctx;
    int error;

    //vap->va_size = 134217728;
    //return 0;

    //    panic("take this");
    //printf("VOP_GETATTR(%p, %p, %d)\n", vp, vap, flags);
    vctx = vfs_context_create((vfs_context_t)0);
    error= vnode_getattr(vp, vap, vctx);
    (void) vfs_context_rele(vctx);
    return error;
}

#if 1
errno_t VNOP_LOOKUP(struct vnode *, struct vnode **, struct componentname *, vfs_context_t);

errno_t VOP_LOOKUP(struct vnode *vp, struct vnode **vpp, struct componentname *cn, vfs_context_t ct)
{
    return VNOP_LOOKUP(vp,vpp,cn,ct);
}
#endif
#if 0
extern errno_t VNOP_MKDIR   (struct vnode *, struct vnode **,
                             struct componentname *, struct vnode_attr *,
                             vfs_context_t);
errno_t VOP_MKDIR(struct vnode *vp, struct vnode **vpp,
                  struct componentname *cn, struct vnode_attr *vattr,
                  vfs_context_t ct)
{
    return VNOP_MKDIR(vp, vpp, cn, vattr, ct);
}

extern errno_t VNOP_REMOVE  (struct vnode *, struct vnode *,
                             struct componentname *, int, vfs_context_t);
errno_t VOP_REMOVE  (struct vnode *vp, struct vnode *dp,
                             struct componentname *cn, int flags,
                      vfs_context_t ct)
{
    return VNOP_REMOVE(vp, dp, cn, flags, ct);
}


extern errno_t VNOP_SYMLINK (struct vnode *, struct vnode **,
                             struct componentname *, struct vnode_attr *,
                             char *, vfs_context_t);
errno_t VOP_SYMLINK (struct vnode *vp, struct vnode **vpp,
                             struct componentname *cn, struct vnode_attr *attr,
                             char *name, vfs_context_t ct)
{
    return VNOP_SYMLINK(vp, vpp, cn, attr, name, ct);
}
#endif


#undef VFS_ROOT

extern int VFS_ROOT(mount_t, struct vnode **, vfs_context_t);
int spl_vfs_root(mount_t mount, struct vnode **vp)
{
    return VFS_ROOT(mount, vp, vfs_context_current() );
}



void vfs_mountedfrom(struct mount *vfsp, char *osname)
{
    (void) copystr(osname, vfs_statfs(vfsp)->f_mntfromname, MNAMELEN - 1, 0);
}


/*
 * DNLC Name Cache Support
 */
struct vnode *
dnlc_lookup(struct vnode *dvp, char *name)
{
    struct componentname cn;
	struct vnode *vp;

    //return DNLC_NO_VNODE;
	bzero(&cn, sizeof (cn));
	cn.cn_nameiop = LOOKUP;
	cn.cn_flags = ISLASTCN;
	cn.cn_nameptr = (char *)name;
	cn.cn_namelen = strlen(name);

	switch(cache_lookup(dvp, &vp, &cn)) {
	case -1:
		break;
	case ENOENT:
		vp = DNLC_NO_VNODE;
		break;
	default:
		vp = NULLVP;
	}
	return (vp);
}

int dnlc_purge_vfsp(struct mount *mp, int flags)
{
    cache_purgevfs(mp);
    return 0;
}

void dnlc_remove(struct vnode *vp, char *name)
{
    cache_purge(vp);
    return;
}


/*
 *
 *
 */
void dnlc_update(struct vnode *vp, char *name, struct vnode *tp)
{

#if 0
    // If tp is NULL, it is a negative-cache entry
    struct componentname cn;

    // OSX panics if you give empty(non-NULL) name
    if (!name || !*name || !strlen(name)) return;

	bzero(&cn, sizeof (cn));
	cn.cn_nameiop = CREATE;
	cn.cn_flags = ISLASTCN;
	cn.cn_nameptr = (char *)name;
	cn.cn_namelen = strlen(name);

    cache_enter(vp, tp==DNLC_NO_VNODE?NULL:tp, &cn);
#endif
    return;
}



static kmutex_t spl_getf_lock;
static list_t   spl_getf_list;


int spl_vnode_init(void)
{
    mutex_init(&spl_getf_lock, NULL, MUTEX_DEFAULT, NULL);
    list_create(&spl_getf_list, sizeof (struct spl_fileproc),
		offsetof(struct spl_fileproc, f_next));
    return 0;
}

void spl_vnode_fini(void)
{
    mutex_destroy(&spl_getf_lock);
    list_destroy(&spl_getf_list);
}

#include <sys/file.h>
struct fileproc;

extern int fp_drop(struct proc *p, int fd, struct fileproc *fp, int locked);
extern int fp_drop_written(struct proc *p, int fd, struct fileproc *fp,
                           int locked);
extern int fp_lookup(struct proc *p, int fd, struct fileproc **resultfp, int locked);
extern int fo_read(struct fileproc *fp, struct uio *uio, int flags,
                   vfs_context_t ctx);
extern int fo_write(struct fileproc *fp, struct uio *uio, int flags,
                    vfs_context_t ctx);
extern int file_vnode_withvid(int, struct vnode **, uint32_t *);
extern int file_drop(int);

#if ZFS_LEOPARD_ONLY
#define file_vnode_withvid(a, b, c) file_vnode(a, b)
#endif


/*
 * getf(int fd) - hold a lock on a file descriptor, to be released by calling
 * releasef(). On OSX we will also look up the vnode of the fd for calls
 * to spl_vn_rdwr().
 */
void *getf(int fd)
{
    struct fileproc     *fp  = NULL;
    struct spl_fileproc *sfp = NULL;
	struct vnode *vp;
	uint32_t vid;

    /*
     * We keep the "fp" pointer as well, both for unlocking in releasef() and
     * used in vn_rdwr().
     */

    sfp = kmem_alloc(sizeof(*sfp), KM_SLEEP);
    if (!sfp) return NULL;

    if (fp_lookup(current_proc(), fd, &fp, 0/*!locked*/)) {
        kmem_free(sfp, sizeof(*sfp));
        return (NULL);
    }

    /*
     * The f_vnode ptr is used to point back to the "sfp" node itself, as it is
     * the only information passed to vn_rdwr.
     */
    sfp->f_vnode  = sfp;
    sfp->f_fd     = fd;
    sfp->f_offset = 0;
    sfp->f_proc   = current_proc();
    sfp->f_fp     = fp;

	/* Also grab vnode, so we can fish out the minor, for onexit */
	if (!file_vnode_withvid(fd, &vp, &vid)) {
		if (vnode_vtype(vp) != VDIR) {
			sfp->f_file = minor(vnode_specrdev(vp));
		}
		file_drop(fd);
	}

	mutex_enter(&spl_getf_lock);
	list_insert_tail(&spl_getf_list, sfp);
	mutex_exit(&spl_getf_lock);

    //printf("SPL: new getf(%d) ret %p fp is %p so vnode set to %p\n",
    //     fd, sfp, fp, sfp->f_vnode);

    return sfp;
}

struct vnode *getf_vnode(void *fp)
{
	struct spl_fileproc *sfp = (struct spl_fileproc *) fp;
	struct vnode *vp = NULL;
	uint32_t vid;

	if (!file_vnode_withvid(sfp->f_fd, &vp, &vid)) {
		file_drop(sfp->f_fd);
	}

	return vp;
}

void releasef(int fd)
{
    struct spl_fileproc *fp = NULL;
    struct proc *p;

    //printf("SPL: releasef(%d)\n", fd);

    p = current_proc();
	mutex_enter(&spl_getf_lock);
	for (fp = list_head(&spl_getf_list); fp != NULL;
	     fp = list_next(&spl_getf_list, fp)) {
        if ((fp->f_proc == p) && fp->f_fd == fd) break;
    }
	mutex_exit(&spl_getf_lock);
    if (!fp) return; // Not found

    //printf("SPL: releasing %p\n", fp);

    // Release the hold from getf().
    if (fp->f_writes)
        fp_drop_written(p, fd, fp->f_fp, 0/*!locked*/);
    else
        fp_drop(p, fd, fp->f_fp, 0/*!locked*/);

    // Remove node from the list
	mutex_enter(&spl_getf_lock);
	list_remove(&spl_getf_list, fp);
	mutex_exit(&spl_getf_lock);

    // Free the node
    kmem_free(fp, sizeof(*fp));

}



/*
 * Our version of vn_rdwr, here "vp" is not actually a vnode, but a ptr
 * to the node allocated in getf(). We use the "fp" part of the node to
 * be able to issue IO.
 * You must call getf() before calling spl_vn_rdwr().
 */
int spl_vn_rdwr(enum uio_rw rw,
                struct vnode *vp,
                caddr_t base,
                ssize_t len,
                offset_t offset,
                enum uio_seg seg,
                int ioflag,
                rlim64_t ulimit,    /* meaningful only if rw is UIO_WRITE */
                cred_t *cr,
                ssize_t *residp)
{
    struct spl_fileproc *sfp = (struct spl_fileproc*)vp;
    uio_t *auio;
    int spacetype;
    int error=0;
    vfs_context_t vctx;

    spacetype = UIO_SEG_IS_USER_SPACE(seg) ? UIO_USERSPACE32 : UIO_SYSSPACE;

    vctx = vfs_context_create((vfs_context_t)0);
    auio = uio_create(1, 0, spacetype, rw);
    uio_reset(auio, offset, spacetype, rw);
    uio_addiov(auio, (uint64_t)(uintptr_t)base, len);

    if (rw == UIO_READ) {
        error = fo_read(sfp->f_fp, auio, ioflag, vctx);
    } else {
        error = fo_write(sfp->f_fp, auio, ioflag, vctx);
        sfp->f_writes = 1;
    }

    if (residp) {
        *residp = uio_resid(auio);
    } else {
        if (uio_resid(auio) && error == 0)
            error = EIO;
    }

    uio_free(auio);
    vfs_context_rele(vctx);

    return (error);
}

void spl_rele_async(void *arg)
{
    struct vnode *vp = (struct vnode *)arg;
    if (vp) vnode_put(vp);
}

void vn_rele_async(struct vnode *vp, void *taskq)
{
	VERIFY(taskq_dispatch((taskq_t *)taskq,
						  (task_func_t *)spl_rele_async, vp, TQ_SLEEP) != 0);
}



vfs_context_t spl_vfs_context_kernel(void)
{
	return vfs_context_kernel();
}

#undef build_path
extern int build_path(struct vnode *vp, char *buff, int buflen, int *outlen,
					  int flags, vfs_context_t ctx);

int spl_build_path(struct vnode *vp, char *buff, int buflen, int *outlen,
				   int flags, vfs_context_t ctx)
{
	return build_path(vp, buff, buflen, outlen, flags, ctx);
}

/*
 * vnode_notify was moved from KERNEL_PRIVATE to KERNEL in 10.11, but to be
 * backward compatible, we keep the wrapper for now.
 */
extern int vnode_notify(struct vnode *, uint32_t, struct vnode_attr*);
int spl_vnode_notify(struct vnode *vp, uint32_t type, struct vnode_attr *vap)
{
	return vnode_notify(vp, type, vap);
}

extern int	vfs_get_notify_attributes(struct vnode_attr *vap);
int	spl_vfs_get_notify_attributes(struct vnode_attr *vap)
{
	return vfs_get_notify_attributes(vap);
}

/* Root directory vnode for the system a.k.a. '/' */
/* Must use vfs_rootvnode() to acquire a reference, and
 * vnode_put() to release it
 */

extern struct vnode *rootvnode;

struct vnode *
getrootdir(void)
{
	struct vnode *rvnode;

	// Unfortunately, Apple's vfs_rootvnode() fails to check for
	// NULL rootvp, and just panics. We aren't technically allowed to
	// see rootvp, but in the interest of avoiding a panic...
	if (rootvnode == NULL) return NULL;

	rvnode = vfs_rootvnode();
	if (rvnode)
		vnode_put(rvnode);
	return rvnode;
}


/*
 * It is undesirable to shadow kernel structure here, but since the
 * alloc calls decmpfs_cnode_alloc() and decmpfs_cnode_free() were not
 * added until 10.12, we simplify the "zfs.export" files but doing this.
 * Once we drop 10.11 support, we should change to calling those functions.
 *
 * Warning: 10.15 changed decmpfs_cnode! We will use cnode_alloc() from
 * this version onward.
 */
#include <AvailabilityMacros.h>

#if defined (MAC_OS_X_VERSION_10_12) && \
	(MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
/* Use internal XNU struct from 10.12 (required at 10.15) */
#else
// Up to 10.11, use shadow struct.
typedef struct decmpfs_cnode {
	uint8_t cmp_state;
	uint8_t cmp_minimal_xattr;
	uint32_t cmp_type;
	uint32_t lockcount;
	void	*lockowner;
	uint64_t uncompressed_size __attribute__((aligned(8)));
	/* 10.15: nchildren, total_size */
	uint64_t decompression_flags;
	/*lck_rw_t compressed_data_lock;*/
	uint32_t compressed_data_lock[4];
} decmpfs_cnode;
#endif

struct decmpfs_cnode *decmpfs_cnode_alloc(void);
struct decmpfs_cnode *spl_decmpfs_cnode_alloc(void)
{
#if defined (MAC_OS_X_VERSION_10_12) && \
	(MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
	return decmpfs_cnode_alloc();
#else
	struct decmpfs_cnode *dp;
	dp = kmem_alloc(sizeof(struct decmpfs_cnode), KM_SLEEP);
	return dp;
#endif
}

void decmpfs_cnode_free(struct decmpfs_cnode *dp);
void spl_decmpfs_cnode_free(struct decmpfs_cnode *dp)
{
#if defined (MAC_OS_X_VERSION_10_12) && \
	(MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
	decmpfs_cnode_free(dp);
#else
	kmem_free(dp, sizeof(struct decmpfs_cnode));
#endif
}

int decmpfs_decompress_file(struct vnode *vp, struct decmpfs_cnode *cp,
	off_t toSize, int truncate_okay, int skiplock);
int spl_decmpfs_decompress_file(struct vnode *vp, struct decmpfs_cnode *cp,
	off_t toSize, int truncate_okay, int skiplock)
{
	return decmpfs_decompress_file(vp, cp, toSize, truncate_okay, skiplock);
}

int decmpfs_file_is_compressed(struct vnode *vp, struct decmpfs_cnode *cp);
int spl_decmpfs_file_is_compressed(struct vnode *vp, struct decmpfs_cnode *cp)
{
	return decmpfs_file_is_compressed(vp, cp);
}

void decmpfs_cnode_init(struct decmpfs_cnode *cp);
void spl_decmpfs_cnode_init(struct decmpfs_cnode *cp)
{
	decmpfs_cnode_init(cp);
}

void decmpfs_cnode_destroy(struct decmpfs_cnode *cp);
void spl_decmpfs_cnode_destroy(struct decmpfs_cnode *cp)
{
	decmpfs_cnode_destroy(cp);
}
