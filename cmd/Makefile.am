SUBDIRS  = zfs zpool zdb zhack zinject zstream ztest
SUBDIRS += fsck_zfs vdev_id raidz_test zfs_ids_to_path
SUBDIRS += zpool_influxdb

CPPCHECKDIRS  = zfs zpool zdb zhack zinject zstream ztest
CPPCHECKDIRS += raidz_test zfs_ids_to_path zpool_influxdb

if USING_PYTHON
SUBDIRS += arcstat arc_summary dbufstat
endif

if BUILD_LINUX
SUBDIRS += mount_zfs zed zgenhostid zvol_id zvol_wait
CPPCHECKDIRS += mount_zfs zed zgenhostid zvol_id
endif

# Eventually "os" will be "built" by all platforms?
if BUILD_MACOS
SUBDIRS += zed os
endif
