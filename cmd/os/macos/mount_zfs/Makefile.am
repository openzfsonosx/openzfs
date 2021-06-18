include $(top_srcdir)/config/Rules.am

sbin_PROGRAMS = mount_zfs

mount_zfs_SOURCES = \
	mount_zfs.c

mount_zfs_LDADD = \
	$(abs_top_builddir)/lib/libzfs/libzfs.la \
	$(abs_top_builddir)/lib/libzfs_core/libzfs_core.la \
	$(abs_top_builddir)/lib/libnvpair/libnvpair.la

mount_zfs_LDADD += $(LTLIBINTL)

include $(top_srcdir)/config/CppCheck.am
