dnl #
dnl # Check for BSD -lfetch
dnl #
AC_DEFUN([ZFS_AC_CONFIG_USER_BSD_LIBFETCH], [
	AC_SEARCH_LIBS([fetchParseURL], [fetch], [
		AC_DEFINE([HAVE_BSD_FETCH], 1,
			[Define if you have a BSD-compatible libfetch])
		])
])
