#!/bin/sh
#
# Helper to mount and unmount snapshots when asked to by kernel.
#
# Mostly used in macOS.
#
set -ef

[ -f "${ZED_ZEDLET_DIR}/zed.rc" ] && . "${ZED_ZEDLET_DIR}/zed.rc"
. "${ZED_ZEDLET_DIR}/zed-functions.sh"

[ -n "${ZEVENT_SNAPSHOT_NAME}" ] || exit 9
[ -n "${ZEVENT_SUBCLASS}" ] || exit 9

if   [ "${ZEVENT_SUBCLASS}" = "snapshot_mount" ]; then
    action="mount"
elif [ "${ZEVENT_SUBCLASS}" = "snapshot_unmount" ]; then
    action="unmount"
else
    zed_log_err "unsupported event class \"${ZEVENT_SUBCLASS}\""
    exit 9
fi

zed_exit_if_ignoring_this_event
zed_check_cmd "${ZFS}"

"${ZFS}" $"{action}" "${ZEVENT_SNAPSHOT_NAME}"

finished
