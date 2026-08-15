# PresenceOFX v1.0.5

CPU image-presence engine, universal arm64+x86_64 GitHub build, and tag-based release ZIP.

## Build fix

- Fixed Xcode 16.4 Metal compile failure caused by invalid `-std=metal2.4`.
- macOS shaders now compile with `-std=macos-metal2.4`.
- CI source sanity rejects the unqualified `metal2.4` flag.

## Packaging fix
- Workflow runs the build through `bash scripts/build_macos.sh`.
- Workflow restores executable bits with `chmod +x`.
- Release ZIP preserves Unix executable mode bits.

## v1.0.5 build hardening

- Removed explicit Metal language-standard pin; active Xcode chooses the compatible macOS default.
- Added named post-CMake build stages so failures are no longer silent.
- Replaced fragile `find | head` bundle lookup with `find -print -quit`.
- Replaced Foundation `MAX`/`MIN` macros in Objective-C++ bridge to remove GNU-extension warnings.
- Workflow invokes the build through `bash` and restores script permissions.

## v1.0.5 Resolve load fix
- Fixed CFBundleExecutable to match the actual Mach-O name: PresenceOFX.ofx.
- Build now verifies Info.plist and binary basename match before packaging.
- Added CI host-load bundle validation.

## v1.0.6 Resolve stability fix
- Disabled the unproven Resolve Metal OFX path; the plugin now advertises and uses the CPU OpenFX renderer only.

## v1.0.7 Metal render restore
- Restored OpenFX 1.5 Metal rendering behind the `OfxImageEffectPropMetalEnabled` gate.
- Kept CPU fallback for non-Metal render actions.
