#!/usr/bin/env bash
set -euo pipefail
BUNDLE="${1:?usage: validate_bundle_macos.sh /path/to/PresenceOFX.ofx.bundle}"
test -d "$BUNDLE"
PLIST="$BUNDLE/Contents/Info.plist"
plutil -lint "$PLIST"
EXEC="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$PLIST")"
test "$EXEC" = "PresenceOFX.ofx"
BIN="$BUNDLE/Contents/MacOS/$EXEC"
test -f "$BIN"
ARCHS="$(lipo -archs "$BIN")"
case " $ARCHS " in *" arm64 "*) ;; *) exit 1;; esac
case " $ARCHS " in *" x86_64 "*) ;; *) exit 1;; esac
nm -gU "$BIN" | grep -q '_OfxGetNumberOfPlugins'
nm -gU "$BIN" | grep -q '_OfxGetPlugin'
nm -gU "$BIN" | grep -q '_OfxSetHost'
codesign --verify --deep --strict "$BUNDLE"
echo "Host-load bundle sanity passed."
