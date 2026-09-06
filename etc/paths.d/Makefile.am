# SPDX-License-Identifier: CDDL-1.0
pathsddir = $(sysconfdir)/paths.d
pathsd_DATA = %D%/zfs-path

# zfs-path is generated from zfs-path.in, so it must not be a dist_ file:
# automake distributes those even from inside "if BUILD_MACOS", and nothing
# generates the file on other platforms ("No rule to make target ...").
SUBSTFILES += $(pathsd_DATA)

CLEANFILES += $(pathsd_DATA)
CLEANFILES += %D%/zfs-path

INSTALL_DATA_HOOKS += pathsd-data-hook
pathsd-data-hook:
	-$(SED) -e 's,@bindir\@,$(bindir),g' \
		-e 's,@runstatedir\@,$(runstatedir),g' \
		-e 's,@sbindir\@,$(sbindir),g' \
		-e 's,@sysconfdir\@,$(sysconfdir),g' \
		-e 's,@launchddaemondir\@,$(launchddaemondir),g' \
		-e 's,@launchdscriptdir\@,$(launchdscriptdir),g' \
		%D%/zfs-path.in > $(DESTDIR)/$(sysconfdir)/paths.d/zfs-path
