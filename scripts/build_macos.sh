#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

command -v cmake >/dev/null
command -v clang++ >/dev/null
xcrun --find metal >/dev/null
xcrun --find metallib >/dev/null
command -v lipo >/dev/null

rm -rf build
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel "$(sysctl -n hw.ncpu)"

xcrun metal -std=metal2.4 -O3 -c shaders/PresenceKernels.metal -o build/PresenceKernels.air
xcrun metallib build/PresenceKernels.air -o build/PresenceKernels.metallib

BUNDLE="build/PresenceOFX.ofx.bundle"
if [ ! -d "$BUNDLE" ]; then
  # CMake can place bundles in different dirs depending on generator.
  BUNDLE="$(find build -name 'PresenceOFX.ofx.bundle' -type d | head -n 1)"
fi
[ -n "$BUNDLE" ] && [ -d "$BUNDLE" ]
mkdir -p "$BUNDLE/Contents/Resources"
cp build/PresenceKernels.metallib "$BUNDLE/Contents/Resources/PresenceKernels.metallib"

BIN="$BUNDLE/Contents/MacOS/PresenceOFX.ofx"
[ -f "$BIN" ]
file "$BIN"
ARCHS="$(lipo -archs "$BIN")"
echo "Architectures: $ARCHS"
echo "$ARCHS" | grep -q arm64
echo "$ARCHS" | grep -q x86_64
nm -gU "$BIN" | grep _OfxGetNumberOfPlugins
nm -gU "$BIN" | grep _OfxGetPlugin
nm -gU "$BIN" | grep _OfxSetHost

if otool -L "$BIN" | grep -E '/opt/homebrew|/usr/local/opt'; then
  echo "Unexpected Homebrew runtime dependency" >&2
  exit 1
fi

codesign --force --sign - "$BUNDLE"
codesign --verify --deep --strict "$BUNDLE"

rm -rf dist
mkdir -p dist
DITTO_OPTS=(--keepParent -c -k --sequesterRsrc --zlibCompressionLevel 9)
ditto "${DITTO_OPTS[@]}" "$BUNDLE" "dist/PresenceOFX-macOS-universal.zip"
echo "Built dist/PresenceOFX-macOS-universal.zip"
