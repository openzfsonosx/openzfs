include $(top_srcdir)/config/Rules.am

DEFAULT_INCLUDES += -I$(srcdir)

noinst_LTLIBRARIES = libshare.la

USER_C = \
	libshare_impl.h \
	libshare.c \
	nfs.c \
	nfs.h \
	smb.h

if BUILD_LINUX
USER_C += \
	os/linux/nfs.c \
	os/linux/smb.c
endif

if BUILD_FREEBSD
USER_C += \
	os/freebsd/nfs.c \
	os/freebsd/smb.c
endif

if BUILD_MACOS
USER_C += \
	os/macos/nfs.c \
	os/macos/smb.c
endif

libshare_la_SOURCES = $(USER_C)

include $(top_srcdir)/config/CppCheck.am
