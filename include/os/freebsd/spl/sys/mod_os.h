#ifndef _SPL_MOD_H
#define	_SPL_MOD_H

#define	ZFS_MODULE_DESCRIPTION(s)
#define	ZFS_MODULE_AUTHOR(s)
#define	ZFS_MODULE_LICENSE(s)
#define	ZFS_MODULE_VERSION(s)

#define ZFS_MODULE_PARAM_CALL(scope_prefix, name_prefix, name, setfunc, getfunc, perm, desc)

#include <sys/kernel.h>
#define module_init(fn)							\
static void \
wrap_ ## fn(void *dummy __unused) \
{								 \
	fn();						 \
}																		\
 SYSINIT(zfs_ ## fn, SI_SUB_LAST, SI_ORDER_FIRST, wrap_ ## fn, NULL)


#define module_exit(fn) 							\
static void \
wrap_ ## fn(void *dummy __unused) \
{								 \
	fn();						 \
}																		\
SYSUNINIT(zfs_ ## fn, SI_SUB_LAST, SI_ORDER_FIRST, wrap_ ## fn, NULL)

#endif /* SPL_MOD_H */
