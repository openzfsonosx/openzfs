pathsddir = $(sysconfdir)/manpaths.d
pathsd_DATA = zfs

EXTRA_DIST = \
	zfs

zfs: zfs.in

$(pathsd_DATA):
	-$(SED) -e 's,@mandir\@,$(mandir),g' \
		'$@.in' >'$@'

distclean-local::
	-$(RM) $(pathsd_DATA)
