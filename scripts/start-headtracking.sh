#!/usr/bin/env bash
# Start head tracking for opentrack: the USB service, Tobii Engine and the
# bridge that sends head pose to opentrack's "UDP over network" input.
# Runs until stopped with Ctrl+C; stopping it stops all three.
#
# Usage: scripts/start-headtracking.sh [--port 4242] [--print]
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
exec "$repo/scripts/run-sandboxed.sh" "$repo/scripts/with-engine.sh" "$repo/build/tobii_opentrack" "$@"
