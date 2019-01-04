#ifndef _LINUX_ZFS_IOCTL_OS_H_
#define	_LINUX_ZFS_IOCTL_OS_H_

typedef int zfs_ioc_legacy_func_t(zfs_cmd_t *);
typedef int zfs_ioc_func_t(const char *, nvlist_t *, nvlist_t *);
typedef int zfs_secpolicy_func_t(zfs_cmd_t *, nvlist_t *, cred_t *);

typedef enum {
	POOL_CHECK_NONE		= 1 << 0,
	POOL_CHECK_SUSPENDED	= 1 << 1,
	POOL_CHECK_READONLY	= 1 << 2,
} zfs_ioc_poolcheck_t;

typedef enum {
	NO_NAME,
	POOL_NAME,
	DATASET_NAME,
	ENTITY_NAME
} zfs_ioc_namecheck_t;

/*
 * IOC Keys are used to document and validate user->kernel interface inputs.
 * See zfs_keys_recv_new for an example declaration. Any key name that is not
 * listed will be rejected as input.
 *
 * The keyname 'optional' is always allowed, and must be an nvlist if present.
 * Arguments which older kernels can safely ignore can be placed under the
 * "optional" key.
 *
 * When adding new keys to an existing ioc for new functionality, consider:
 * 	- adding an entry into zfs_sysfs.c zfs_features[] list
 * 	- updating the libzfs_input_check.c test utility
 *
 * Note: in the ZK_WILDCARDLIST case, the name serves as documentation
 * for the expected name (bookmark, snapshot, property, etc) but there
 * is no validation in the preflight zfs_check_input_nvpairs() check.
 */
typedef enum {
	ZK_OPTIONAL = 1 << 0,		/* pair is optional */
	ZK_WILDCARDLIST = 1 << 1,	/* one or more unspecified key names */
} ioc_key_flag_t;

typedef struct zfs_ioc_key {
	const char	*zkey_name;
	data_type_t	zkey_type;
	ioc_key_flag_t	zkey_flags;
} zfs_ioc_key_t;

int zfs_secpolicy_config(zfs_cmd_t *zc, nvlist_t *innvl, cred_t *cr);

/* BEGIN CSTYLED */
void zfs_ioctl_register_dataset_nolog(zfs_ioc_t ioc, zfs_ioc_legacy_func_t *func,
    zfs_secpolicy_func_t *secpolicy, zfs_ioc_poolcheck_t pool_check);

void zfs_ioctl_register(const char *name, zfs_ioc_t ioc, zfs_ioc_func_t *func,
    zfs_secpolicy_func_t *secpolicy, zfs_ioc_namecheck_t namecheck,
    zfs_ioc_poolcheck_t pool_check, boolean_t smush_outnvlist,
    boolean_t allow_log, const zfs_ioc_key_t *nvl_keys, size_t num_keys);
/* END CSTYLED */

void zfs_ioctl_init_os(void);

int zfs_ioc_destroy_snaps(const char *poolname, nvlist_t *innvl,
    nvlist_t *outnvl);

#endif
