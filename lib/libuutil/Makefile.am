include $(top_srcdir)/config/Rules.am

lib_LTLIBRARIES = libuutil.la

include $(top_srcdir)/config/Abigail.am

USER_C = \
	uu_alloc.c \
	uu_avl.c \
	uu_ident.c \
	uu_list.c \
	uu_misc.c \
	uu_pname.c \
	uu_string.c

libuutil_la_SOURCES = $(USER_C)

libuutil_la_LIBADD = \
	$(abs_top_builddir)/lib/libavl/libavl.la \
	$(abs_top_builddir)/lib/libspl/libspl.la

libuutil_la_LIBADD += $(LTLIBINTL)

libuutil_la_LDFLAGS = -pthread

if !BUILD_MACOS
if !ASAN_ENABLED
libuutil_la_LDFLAGS += -Wl,-z,defs
endif
endif

if BUILD_MACOS
libuutil_la_LIBADD += \
	$(abs_top_builddir)/lib/libefi/libefi.la
endif

libuutil_la_LDFLAGS += -version-info 3:0:0

include $(top_srcdir)/config/CppCheck.am

# Library ABI
EXTRA_DIST = libuutil.abi libuutil.suppr
