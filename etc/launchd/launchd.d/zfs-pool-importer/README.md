# zfs-pool-importer

An improved tool to automatically import ZFS pools from [OpenZFS](https://github.com/openzfsonosx/openzfs) when macOS starts up.

The advantages over the [shell script](https://github.com/openzfsonosx/openzfs/blob/macOS/etc/launchd/launchd.d/zpool-import-all.sh.in) are that this command shows up by itself in the list of programs that have requested full disk access, and that `/bin/bash` will not be required to have full disk access.

What's required for `zfs-pool-importer` to request full disk access to happen is that it must be launched once; the operation that requires full disk access will fail, and `zfs-pool-importer` will appear in the list at "System Preferences -> Security & Privacy -> Full Disk Access", unchecked. The user can then choose to enable it. The installation script in this repository performs the necessary launch.

There is no real difference in functionality compared to the shell script, though the log output is slightly different and, as described, the experience is clearer.

### Notes

This tool requires macOS 10.15 (Catalina). Installers for older versions should continue to use the shell script, while installers for 10.15 and newer should use this tool in order to automatically import zfs pools. 
