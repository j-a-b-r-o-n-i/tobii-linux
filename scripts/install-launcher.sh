#!/usr/bin/env bash
# Add a "Head tracking" launcher to the application menu. It runs
# scripts/headtracking-toggle.sh, which starts or stops head tracking.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
dest=${XDG_DATA_HOME:-$HOME/.local/share}/applications/tobii-headtracking.desktop
mkdir -p "$(dirname "$dest")"
cat >"$dest" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Head tracking
GenericName=Tobii head tracking
Comment=Start or stop Tobii head tracking for opentrack
Exec=$repo/scripts/headtracking-toggle.sh
Icon=view-visible
Terminal=false
Categories=Game;
StartupNotify=false
DESKTOP
echo "installed $dest"
