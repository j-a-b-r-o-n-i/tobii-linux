#!/usr/bin/env bash
# Start head tracking if it is off, stop it if it is on. Meant for a desktop
# launcher: reports through desktop notifications and also opens opentrack
# when starting, if it isn't open already.
set -uo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
log=$repo/build/headtracking.log
icon=view-visible

notify() { notify-send -a "Head tracking" -i "$icon" "$@"; }

bridge=$(pgrep -x tobii_opentrack || true)
if [ -n "$bridge" ]; then
  # Stopping the bridge makes the scripts around it stop the engine and the
  # USB service.
  kill $bridge
  for _ in $(seq 20); do pgrep -f 'sbin/tobiiusbserviced$' >/dev/null || break; sleep 0.5; done
  notify "Head tracking stopped"
  exit 0
fi

mkdir -p "$repo/build"
setsid "$repo/scripts/start-headtracking.sh" >"$log" 2>&1 < /dev/null &
started=$!  # the script execs into the sandbox, so follow the PID, not the name
pgrep -x opentrack >/dev/null || setsid opentrack >/dev/null 2>&1 < /dev/null &
notify "Head tracking starting" "This takes about 10 seconds."

for _ in $(seq 60); do
  sleep 1
  if grep -q 'engine running' "$log"; then
    notify "Head tracking on" "Press Start in opentrack if it isn't running. Click the launcher again to stop."
    exit 0
  fi
  if grep -q 'engine never connected' "$log" || ! kill -0 "$started" 2>/dev/null; then
    break
  fi
done
notify -u critical "Head tracking didn't start" "Unplug the tracker, plug it back in, and click the launcher again. Details: $log"
exit 1
