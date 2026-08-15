# PresenceOFX v1.0.2

Metal image-presence engine with direct Resolve MTLBuffer processing, universal arm64+x86_64 GitHub build, xcrun Metal compiler, CPU fallback only on CPU renders, and tag-based release ZIP.

## Build fix

- Fixed Xcode 16.4 Metal compile failure caused by invalid `-std=metal2.4`.
- macOS shaders now compile with `-std=macos-metal2.4`.
- CI source sanity rejects the unqualified `metal2.4` flag.
