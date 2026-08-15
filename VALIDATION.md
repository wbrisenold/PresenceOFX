# Validation Notes

Performed in the local Linux container:

- CPU model tests compiled and passed.
- `PresenceOFX.cpp` compiled as C++ syntax-only/object test against the pinned minimal OFX ABI header.
- No external OpenFX SDK is required.
- No Homebrew dependency is declared.

Remaining required validation in GitHub Actions / Resolve:

- Apple Metal compiler must compile `PresenceKernels.metal`.
- Universal macOS bundle must pass `lipo` architecture checks.
- OFX export symbols must pass `nm` checks.
- Bundle must ad-hoc sign successfully.
- DaVinci Resolve must load the plugin and render test clips without crash.

The first GitHub Actions macOS run is the real Metal compiler gate.
