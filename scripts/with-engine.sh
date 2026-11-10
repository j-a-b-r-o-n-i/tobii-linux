#!/usr/bin/env bash
# Start Tobii Engine, run a client command, then stop the engine.
# Meant to run inside scripts/run-sandboxed.sh, after the USB service is up:
#   scripts/run-sandboxed.sh scripts/with-engine.sh build/probe 8
# The engine runs from vendor/engine-run, a copy with the firmware folder
# removed so it has nothing it could flash onto the tracker.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
eng=$repo/vendor/engine-run
deps=$repo/vendor/deps/usr/lib/x86_64-linux-gnu
log=$repo/build/engine.log

# local/engine-env.sh (ignored by git) may set pre=(...), a command prefix
# for the engine, for machine-specific settings.
pre=()
if [ -f "$repo/local/engine-env.sh" ]; then . "$repo/local/engine-env.sh"; fi

# The tracker often ignores a connection made soon after the previous one
# closed. The engine tries only once and then crashes, so keep restarting it
# until it starts without that error.
epid=
for attempt in 1 2 3 4 5 6; do
  (cd "$eng/lib" && exec "${pre[@]}" env LD_LIBRARY_PATH=$eng/lib:$eng/platform_modules:$deps "$eng/tobii_engine") >"$log" 2>&1 &
  epid=$!
  sleep "${ENGINE_WAIT:-5}"
  if kill -0 "$epid" 2>/dev/null && ! grep -q NO_RESPONSE "$log"; then
    echo "--- engine running (attempt $attempt)" >&2
    break
  fi
  echo "--- engine did not connect (attempt $attempt), retrying" >&2
  kill "$epid" 2>/dev/null || true
  wait "$epid" 2>/dev/null || true
  epid=
  sleep 2
done
if [ -z "$epid" ]; then echo "--- engine never connected, see $log" >&2; exit 1; fi

status=0; "$@" || status=$?
kill "$epid" 2>/dev/null || true
exit $status
