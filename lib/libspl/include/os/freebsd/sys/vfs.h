#ifndef ZFS_SYS_VFS_H_
#define	ZFS_SYS_VFS_H_

#ifdef __linux__
#include_next <sys/vfs.h>
#else
#ifdef _KERNEL
#include_next <sys/vfs.h>
#else
#include_next <sys/statvfs.h>

int fsshare(const char *, const char *, const char *);
int fsunshare(const char *, const char *);
#endif /* !_KERNEL */

#endif /* !__linux__ */

#endif /* !ZFS_SYS_VFS_H_ */
