include $(top_srcdir)/config/Rules.am

noinst_LTLIBRARIES = libefi.la

if BUILD_LINUX
USER_C = \
	rdwr_efi.c
endif

if BUILD_MACOS
USER_C = \
	rdwr_efi_macos.c
endif

nodist_libefi_la_SOURCES = $(USER_C)

libefi_la_LIBADD = $(LIBUUID)

if BUILD_MACOS
libefi_la_LDFLAGS= -framework DiskArbitration -framework CoreServices
endif

EXTRA_DIST = $(USER_C)
