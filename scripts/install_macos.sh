#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUNDLE="$(find "$ROOT/build" -name 'PresenceOFX.ofx.bundle' -type d | head -n 1)"
if [ -z "$BUNDLE" ]; then
  echo "Build first: scripts/build_macos.sh" >&2
  exit 1
fi
DEST="$HOME/Library/OFX/Plugins"
mkdir -p "$DEST"
rm -rf "$DEST/PresenceOFX.ofx.bundle"
cp -R "$BUNDLE" "$DEST/"
echo "Installed to $DEST/PresenceOFX.ofx.bundle"
echo "Restart DaVinci Resolve."
