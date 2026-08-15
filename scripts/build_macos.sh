#!/usr/bin/env bash
set -euxo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

command -v cmake >/dev/null
command -v clang++ >/dev/null
command -v lipo >/dev/null

rm -rf build dist

echo "== Configure universal OFX =="
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

echo "== Build universal OFX =="
cmake --build build --config Release --parallel "$(sysctl -n hw.ncpu)"

echo "== Locate OFX bundle =="
BUNDLE="build/PresenceOFX.ofx.bundle"
if [ ! -d "$BUNDLE" ]; then
  BUNDLE="$(find build -type d -name 'PresenceOFX.ofx.bundle' -print -quit)"
fi
test -n "$BUNDLE"
test -d "$BUNDLE"
echo "Bundle: $BUNDLE"

echo "== Validate universal binary =="
BIN="$BUNDLE/Contents/MacOS/PresenceOFX"
if [ -f "$BIN" ]; then
  mv "$BIN" "$BUNDLE/Contents/MacOS/PresenceOFX.ofx"
fi
BIN="$BUNDLE/Contents/MacOS/PresenceOFX.ofx"
test -f "$BIN"

echo "== Validate macOS bundle metadata =="
PLIST="$BUNDLE/Contents/Info.plist"
test -f "$PLIST"
PLIST_EXEC="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$PLIST")"
echo "CFBundleExecutable: $PLIST_EXEC"
test "$PLIST_EXEC" = "PresenceOFX.ofx"
test -f "$BUNDLE/Contents/MacOS/$PLIST_EXEC"
plutil -lint "$PLIST"

file "$BIN"
ARCHS="$(lipo -archs "$BIN")"
echo "Architectures: $ARCHS"
case " $ARCHS " in *" arm64 "*) ;; *) echo "Missing arm64" >&2; exit 1;; esac
case " $ARCHS " in *" x86_64 "*) ;; *) echo "Missing x86_64" >&2; exit 1;; esac

echo "== Validate OFX exports =="
SYMS="$(nm -gU "$BIN")"
printf '%s\n' "$SYMS" | grep -q '_OfxGetNumberOfPlugins'
printf '%s\n' "$SYMS" | grep -q '_OfxGetPlugin'
printf '%s\n' "$SYMS" | grep -q '_OfxSetHost'

echo "== Validate runtime dependencies =="
if otool -L "$BIN" | grep -E '/opt/homebrew|/usr/local/opt'; then
  echo "Unexpected Homebrew runtime dependency" >&2
  exit 1
fi

echo "== Sign bundle =="
codesign --force --deep --sign - "$BUNDLE"
codesign --verify --deep --strict "$BUNDLE"

echo "== Package release =="
mkdir -p dist
ditto --keepParent -c -k --sequesterRsrc --zlibCompressionLevel 9 "$BUNDLE" "dist/PresenceOFX-macOS-universal.zip"
test -s dist/PresenceOFX-macOS-universal.zip
echo "Built dist/PresenceOFX-macOS-universal.zip"
