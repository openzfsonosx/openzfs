#/bin/sh

set -e

if [ `/usr/bin/whoami` != "root" ]
then
  /bin/echo "This script must be run as root." >&2
  exit 77 # insufficient permissions
fi

swift build
BUILDDIR=`swift build --show-bin-path`
NAME=zfs-pool-importer

/bin/cp "${BUILDDIR}/${NAME}" "/usr/local/zfs/libexec/zfs/launchd.d/${NAME}"
/bin/cp org.openzfsonosx.zfs-pool-importer.plist /Library/LaunchDaemons

/bin/launchctl load /Library/LaunchDaemons/org.openzfsonosx.zfs-pool-importer.plist

/bin/echo "Please enable full disk access for \"zfs-pool-importer\" in the \"Privacy\" tab of the \"Security & Privacy\" pane of System Preferences."
