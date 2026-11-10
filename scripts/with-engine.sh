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
# The first connection after the USB service starts often gets no answer from
# the tracker, and the engine gives up after one try. Connect once first.
for _ in 1 2 3 4 5; do
  timeout 10 "$repo/build/state" >/dev/null 2>&1 && break
  sleep 1
done
# local/engine-env.sh (ignored by git) may set pre=(...), a command prefix
# for the engine, for machine-specific settings.
pre=()
if [ -f "$repo/local/engine-env.sh" ]; then . "$repo/local/engine-env.sh"; fi
(cd "$eng/lib" && exec "${pre[@]}" env LD_LIBRARY_PATH=$eng/lib:$eng/platform_modules:$deps "$eng/tobii_engine") >"$log" 2>&1 &
epid=$!
sleep "${ENGINE_WAIT:-4}"
if ! kill -0 "$epid" 2>/dev/null; then echo "--- engine exited early, see $log" >&2; fi
status=0; "$@" || status=$?
kill "$epid" 2>/dev/null || true
exit $status
