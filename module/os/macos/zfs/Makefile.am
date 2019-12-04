
INFO_PLIST = Info.plist
PLIST_STRING = InfoPlist.strings

ZFS_META_VERSION = @ZFS_META_VERSION@
ZFS_DEBUG_STR = @ZFS_DEBUG_STR@

zfs_CPPFLAGS = \
	-Wall \
	-nostdinc \
	-mkernel \
	-fno-builtin-printf \
	-D_KERNEL \
	-DKERNEL \
	-DKERNEL_PRIVATE \
	-DDRIVER_PRIVATE \
	-DAPPLE \
	-DNeXT \
	-I$(top_srcdir)/include/os/macos/spl \
	-I$(top_srcdir)/module/icp/include \
	-I$(top_srcdir)/include \
	-I@KERNEL_HEADERS@/Headers \
	-I@KERNEL_HEADERS@/PrivateHeaders

zfs_CFLAGS = -DDUMMY
zfs_CXXFLAGS =

zfs_LDFLAGS = \
	-Xlinker \
	-kext \
	-nostdlib \
	-lkmodc++ \
	-lkmod \
	-lcc_kext

zfs_LDADD = \
	$(top_builddir)/module/os/macos/spl/libspl.la

zfs_LIBS =

# If we don't set this to nothing, it adds "-lz -liconv"
LIBS =

bin_PROGRAMS = zfs.kext
noinst_PROGRAMS = zfs

zfs_kext_SOURCE =

zfs_SOURCES = test.c \
	../../../avl/avl.c

KERNEL_MODDIR=  $(DESTDIR)@KERNEL_MODPREFIX@/zfs.kext

dist_noinst_DATA = $(PLIST_STRING) $(INFO_PLIST)

zfs.kext$(EXEEXT): zfs $(PLIST_STRING) $(INFO_PLIST)
	@echo ""
	@mkdir -p zfs.kext/Contents/Resources/English.lproj zfs.kext/Contents/MacOS
	@cp -f $(INFO_PLIST) zfs.kext/Contents/
	/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(ZFS_META_VERSION)" zfs.kext/Contents/Info.plist
	/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $(ZFS_META_VERSION)" zfs.kext/Contents/Info.plist
	/usr/libexec/PlistBuddy -c "Delete :OSBundleLibraries:net.lundman.kernel.dependencies" zfs.kext/Contents/Info.plist
	/usr/libexec/PlistBuddy -c "Add :OSBundleLibraries:net.lundman.kernel.dependencies.$(ZFS_KERNEL_DEPENDENCIES) string 12.5.0" zfs.kext/Contents/Info.plist
	@cp -f $(PLIST_STRING) zfs.kext/Contents/Resources/English.lproj/
	@cp -f zfs zfs.kext/Contents/MacOS/
	@mkdir -p zfs.kext/Contents/PlugIns/KernelExports.kext/
	@cp -f $(top_srcdir)/module/os/macos/kernel/kernelexports zfs.kext/Contents/PlugIns/KernelExports.kext/KernelExports
	@cp -f $(top_srcdir)/module/os/macos/kernel/Info.plist zfs.kext/Contents/PlugIns/KernelExports.kext/
	/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier net.lundman.kernel.dependencies.$(SPL_KERNEL_DEPENDENCIES)" zfs.kext/Contents/PlugIns/KernelExports.kext/Info.plist
	if [ "0$(ZFS_BOOT)" -eq 1 ]; then /usr/libexec/PlistBuddy -c "Add :OSBundleRequired string Root" zfs.kext/Contents/Info.plist; fi
	cp -f $(top_srcdir)/module/os/macos/kernel/version.plist zfs.kext/Contents/PlugIns/KernelExports.kext/
	@kextlibs -undef-symbols -xml zfs.kext/ || echo "Ignoring errors.."

install-exec-local: zfs.kext
	rm -rf $(KERNEL_MODDIR)
	mkdir -p $(KERNEL_MODDIR)
	rsync -r zfs.kext/ $(KERNEL_MODDIR)
	chown -R root:wheel $(KERNEL_MODDIR) || echo "Unable to chown root:wheel $(KERNEL_MODDIR)"
	@echo
	@echo "To load module: kextload -v $(KERNEL_MODDIR)"
	@echo "To uninstall module: rm -rf $(KERNEL_MODDIR)"
	@echo

uninstall-am:
	rm -rf $(KERNEL_MODDIR)

clean:
	rm -rf zfs.kext/
	rm -f *.o *.lo zfs

