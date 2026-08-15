# PresenceOFX v1.0.4

Metal image-presence engine with direct Resolve MTLBuffer processing, universal arm64+x86_64 GitHub build, xcrun Metal compiler, CPU fallback only on CPU renders, and tag-based release ZIP.

## Build fix

- Fixed Xcode 16.4 Metal compile failure caused by invalid `-std=metal2.4`.
- macOS shaders now compile with `-std=macos-metal2.4`.
- CI source sanity rejects the unqualified `metal2.4` flag.

## Packaging fix
- Workflow runs the build through `bash scripts/build_macos.sh`.
- Workflow restores executable bits with `chmod +x`.
- Release ZIP preserves Unix executable mode bits.

## v1.0.4 build hardening

- Removed explicit Metal language-standard pin; active Xcode chooses the compatible macOS default.
- Added named post-CMake build stages so failures are no longer silent.
- Replaced fragile `find | head` bundle lookup with `find -print -quit`.
- Replaced Foundation `MAX`/`MIN` macros in Objective-C++ bridge to remove GNU-extension warnings.
- Workflow invokes the build through `bash` and restores script permissions.
