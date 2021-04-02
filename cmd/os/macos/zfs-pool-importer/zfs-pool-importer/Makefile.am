include $(top_srcdir)/config/Rules.am

AUTOMAKE_OPTIONS = subdir-objects

libexec_PROGRAMS = zfs-pool-importer

zfs-pool-importer_SOURCES = \
	main.m \
	launch.h \
	launch.m \
	stdout.h \
	stdout.m

zfs-pool-importer_CXXFLAGS = -stdlib=libc
zfs-pool-importer_OBJCXXFLAGS = -stdlib=libc
zfs-pool-importer_LDFLAGS = -static -framework CoreFoundation -framework Foundation
