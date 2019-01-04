#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_znode.h>
#include <sys/zap.h>
#include <sys/spa.h>
#include <sys/spa_impl.h>
#include <sys/vdev.h>
#include <sys/vdev_os.h>
#include <sys/vdev_impl.h>
#include <sys/dmu.h>
#include <sys/dsl_dir.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_deleg.h>
#include <sys/dmu_objset.h>
#include <sys/dmu_impl.h>
#include <sys/dmu_redact.h>
#include <sys/dmu_tx.h>
#include <sys/sunddi.h>
#include <sys/policy.h>
#include <sys/zone.h>
#include <sys/nvpair.h>
#include <sys/pathname.h>
#include <sys/sdt.h>
#include <sys/fs/zfs.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_onexit.h>
#include <sys/zvol.h>
#include <sys/dsl_scan.h>
#include <sys/fm/util.h>
#include <sys/dsl_crypt.h>

#include <sys/dmu_recv.h>
#include <sys/dmu_send.h>
#include <sys/dmu_recv.h>
#include <sys/dsl_destroy.h>
#include <sys/dsl_bookmark.h>
#include <sys/dsl_userhold.h>
#include <sys/zfeature.h>
#include <sys/zcp.h>
#include <sys/zio_checksum.h>
#include <sys/vdev_removal.h>
#include <sys/vdev_trim.h>
#include <sys/vdev_impl.h>
#include <sys/vdev_initialize.h>
#include <sys/zfs_ioctl_impl.h>

int
zfs_vfs_ref(zfsvfs_t **zfvp)
{
	int error = 0;

	if (*zfvp) {
		vfs_ref((*zfvp)->z_vfs);
	} else {
		error = SET_ERROR(ESRCH);
	}

	return (error);
}

int
zfsdev_getminor(int fd, minor_t *minorp)
{
	return (ENOTSUP);
}

static const zfs_ioc_key_t zfs_keys_nextboot[] = {
	{"command",		DATA_TYPE_STRING,	0},
	{ ZPOOL_CONFIG_POOL_GUID,		DATA_TYPE_UINT64,	0},
	{ ZPOOL_CONFIG_GUID,		DATA_TYPE_UINT64,	0}
};

static int
zfs_ioc_jail(zfs_cmd_t *zc)
{

	return (zone_dataset_attach(curthread->td_ucred, zc->zc_name,
	    (int)zc->zc_zoneid));
}

static int
zfs_ioc_unjail(zfs_cmd_t *zc)
{

	return (zone_dataset_detach(curthread->td_ucred, zc->zc_name,
	    (int)zc->zc_zoneid));
}

static int
zfs_ioc_nextboot(const char *unused, nvlist_t *innvl, nvlist_t *outnvl)
{
	char name[MAXNAMELEN];
	spa_t *spa;
	vdev_t *vd;
	char *command;
	uint64_t pool_guid;
	uint64_t vdev_guid;
	int error;

	if (nvlist_lookup_uint64(innvl,
	    ZPOOL_CONFIG_POOL_GUID, &pool_guid) != 0)
		return (EINVAL);
	if (nvlist_lookup_uint64(innvl,
	    ZPOOL_CONFIG_GUID, &vdev_guid) != 0)
		return (EINVAL);
	if (nvlist_lookup_string(innvl,
	    "command", &command) != 0)
		return (EINVAL);

	mutex_enter(&spa_namespace_lock);
	spa = spa_by_guid(pool_guid, vdev_guid);
	if (spa != NULL)
		strcpy(name, spa_name(spa));
	mutex_exit(&spa_namespace_lock);
	if (spa == NULL)
		return (ENOENT);

	if ((error = spa_open(name, &spa, FTAG)) != 0)
		return (error);
	spa_vdev_state_enter(spa, SCL_ALL);
	vd = spa_lookup_by_guid(spa, vdev_guid, B_TRUE);
	if (vd == NULL) {
		(void) spa_vdev_state_exit(spa, NULL, ENXIO);
		spa_close(spa, FTAG);
		return (ENODEV);
	}
	error = vdev_label_write_pad2(vd, command, strlen(command));
	(void) spa_vdev_state_exit(spa, NULL, 0);
	txg_wait_synced(spa->spa_dsl_pool, 0);
	spa_close(spa, FTAG);
	return (error);
}


void
zfs_ioctl_init_os(void)
{
	zfs_ioctl_register_dataset_nolog(ZFS_IOC_JAIL, zfs_ioc_jail,
	    zfs_secpolicy_config, POOL_CHECK_NONE);
	zfs_ioctl_register_dataset_nolog(ZFS_IOC_UNJAIL, zfs_ioc_unjail,
	    zfs_secpolicy_config, POOL_CHECK_NONE);
	zfs_ioctl_register("fbsd_nextboot", ZFS_IOC_NEXTBOOT,
	    zfs_ioc_nextboot, zfs_secpolicy_config, NO_NAME,
	    POOL_CHECK_NONE, B_FALSE, B_FALSE, zfs_keys_nextboot, 3);

}
