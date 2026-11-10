#!/usr/bin/env bash
# Run Tobii's USB service, plus a client command, without root.
#
# The service insists on writing to /var/run (a PID file and its sockets), which
# only root can do. This starts a private user and mount namespace, puts an
# empty writable tmpfs over /run inside it, and runs the service and the client
# there. /run/user and /run/dbus are carried over so desktop apps still reach
# the display and the session bus. Nothing outside the namespace changes, and
# the service stops when the client exits.
#
# Usage: scripts/run-sandboxed.sh <client command> [args...]
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
svc=$repo/vendor/usbservice/usr/local
oldrun=$repo/build/.oldrun
mkdir -p "$oldrun"

exec unshare --user --map-root-user --mount --propagation private -- bash -c '
  set -euo pipefail
  svc=$1 oldrun=$2; shift 2
  mount --rbind /run "$oldrun"
  mount -t tmpfs tmpfs /run
  for d in user dbus; do
    mkdir -p /run/$d
    mount --rbind "$oldrun/$d" /run/$d
  done
  mkdir -p /run/tobiiusb
  LD_LIBRARY_PATH=$svc/lib/tobiiusb "$svc/sbin/tobiiusbserviced"
  sleep 2
  pid=$(pgrep -f "sbin/tobiiusbserviced$" || true)
  echo "--- service pid: ${pid:-not running}" >&2
  status=0; "$@" || status=$?
  [ -n "$pid" ] && kill "$pid" || true
  exit $status
' bash "$svc" "$oldrun" "$@"
