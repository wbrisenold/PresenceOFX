# PresenceOFX Metal Render Design

## Goal
Restore true OpenFX 1.5 Metal rendering while keeping the CPU renderer as fallback.

## Design
PresenceOFX advertises `kOfxImageEffectPropMetalRenderSupported` during describe. During render it checks `kOfxImageEffectPropMetalEnabled` on `inArgs`. Only when that value is `1` does it treat `kOfxImagePropData` as `id<MTLBuffer>` and enqueue work on `kOfxImageEffectPropMetalCommandQueue`. Otherwise it treats `kOfxImagePropData` as CPU float memory and calls `processRGBA`.

## Build And Release
GitHub Actions macOS runners compile `shaders/PresenceKernels.metal` with Xcode command-line tools, package `PresenceKernels.metallib` in the OFX bundle, sign the bundle, upload the artifact, and publish tag releases.

## Validation
Local validation covers source sanity, CPU model tests, bundle metadata, OFX exports, and installed-bundle codesign. GitHub validates the Metal compiler path. Runtime validation is Resolve startup plus applying PresenceOFX to a clip.

## Risk
OpenFX documents Metal buffers only when `MetalEnabled == 1`; the previous crash risk came from using the command queue as the gate. This design keeps CPU fallback for non-Metal renders.
