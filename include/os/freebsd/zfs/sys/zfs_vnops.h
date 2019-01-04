#ifndef _SYS_ZFS_VNOPS_H_
#define	_SYS_ZFS_VNOPS_H_
int dmu_write_pages(objset_t *os, uint64_t object, uint64_t offset,
    uint64_t size, struct vm_page **ppa, dmu_tx_t *tx);
int dmu_read_pages(objset_t *os, uint64_t object, vm_page_t *ma, int count,
    int *rbehind, int *rahead, int last_size);
extern int zfs_remove(vnode_t *dip, char *name, cred_t *cr, int flags);
extern int zfs_mkdir(vnode_t *dip, char *dirname, vattr_t *vap,
    vnode_t **ipp, cred_t *cr, int flags, vsecattr_t *vsecp);
extern int zfs_rmdir(vnode_t *dip, char *name, vnode_t *cwd,
    cred_t *cr, int flags);
extern int zfs_setattr(vnode_t *ip, vattr_t *vap, int flag, cred_t *cr);
extern int zfs_rename(vnode_t *sdip, char *snm, vnode_t *tdip,
    char *tnm, cred_t *cr, int flags);
extern int zfs_symlink(vnode_t *dip, const char *name, vattr_t *vap,
    const char *link, vnode_t **ipp, cred_t *cr, int flags);
extern int zfs_link(vnode_t *tdip, vnode_t *sip,
    char *name, cred_t *cr, int flags);
extern int zfs_space(vnode_t *ip, int cmd, struct flock *bfp, int flag,
    offset_t offset, cred_t *cr);
extern int zfs_create(vnode_t *dip, char *name, vattr_t *vap, int excl,
    int mode, vnode_t **ipp, cred_t *cr, int flag, vsecattr_t *vsecp);
extern int zfs_setsecattr(vnode_t *ip, vsecattr_t *vsecp, int flag,
    cred_t *cr);
ssize_t
zpl_write_common(vnode_t *ip, char *buf, size_t len, loff_t *ppos,
    uio_seg_t segment, int flags, cred_t *cr);
#endif
