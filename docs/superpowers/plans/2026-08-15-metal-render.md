# PresenceOFX Metal Render Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore OpenFX 1.5 Metal rendering while preserving CPU fallback.

**Architecture:** PresenceOFX advertises Metal support during describe, but only uses Metal when the render action has `OfxImageEffectPropMetalEnabled == 1`. CPU renders continue using `processRGBA`. GitHub Actions builds the metallib and release artifact using the macOS runner's Xcode tools.

**Tech Stack:** C++17, Objective-C++, Metal, OpenFX 1.5 GPU render properties, CMake, GitHub Actions.

## Global Constraints

- Do not treat `OfxImagePropData` as `id<MTLBuffer>` unless `OfxImageEffectPropMetalEnabled == 1`.
- Keep CPU fallback for non-Metal render actions.
- Use GitHub Actions macOS runners for Xcode/Metal builds.
- Do not add external dependencies.

---

### Task 1: Restore Metal Build And OFX Gates

**Files:**
- Modify: `src/PresenceOFX_OFX.h`
- Modify: `src/PresenceOFX.cpp`
- Modify: `CMakeLists.txt`
- Modify: `scripts/build_macos.sh`
- Modify: `.github/workflows/build.yml`
- Modify: `ci/source_sanity.py`
- Modify: `ci/validate_bundle_macos.sh`

**Interfaces:**
- Consumes: existing `presence::processRGBA(...)` CPU renderer and `presence::runMetal(...)` bridge.
- Produces: render path where `MetalEnabled == 1` calls `runMetal(...)`; otherwise CPU path runs.

- [ ] **Step 1: Add OpenFX Metal constants**

Add this define to `src/PresenceOFX_OFX.h` next to the existing Metal command queue define:

```cpp
#define kOfxImageEffectPropMetalEnabled "OfxImageEffectPropMetalEnabled"
```

- [ ] **Step 2: Restore Metal source to CMake**

Change `CMakeLists.txt` to use `OBJCXX`, include `src/MetalBridge.mm`, restore OBJCXX compile warnings, and link Apple frameworks:

```cmake
project(PresenceOFX LANGUAGES C CXX OBJCXX)

add_library(PresenceOFX MODULE
  src/PresenceOFX.cpp
  src/PresenceCPU.cpp
  src/MetalBridge.mm
)

target_compile_options(PresenceOFX PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Werror=return-type>
  $<$<COMPILE_LANGUAGE:OBJCXX>:-Wall -Wextra -Wpedantic -Werror=return-type>
)

if(APPLE)
  target_link_libraries(PresenceOFX PRIVATE "-framework Foundation" "-framework Metal" "-framework QuartzCore")
endif()
```

- [ ] **Step 3: Advertise Metal support as OpenFX string property**

In `describe(...)`, set Metal support with `setString`, not `setInt`:

```cpp
setString(props, kOfxImageEffectPropMetalRenderSupported, 0, "true");
```

- [ ] **Step 4: Gate Metal render on MetalEnabled**

In `render(...)`, after fetching `Params p`, insert:

```cpp
int metalEnabled = 0;
if (gProp->propGetInt(inArgs, kOfxImageEffectPropMetalEnabled, 0, &metalEnabled) == kOfxStatOK && metalEnabled) {
  void* metalQ = nullptr;
  const bool hasQueue = gProp->propGetPointer(inArgs, kOfxImageEffectPropMetalCommandQueue, 0, &metalQ) == kOfxStatOK && metalQ;
  if (!hasQueue) {
    gEffect->clipReleaseImage(srcImg);
    gEffect->clipReleaseImage(dstImg);
    return kOfxStatFailed;
  }
  const bool ok = runMetal(metalQ, width, height, srcData, dstData, srcStrideFloats, dstStrideFloats, p);
  gEffect->clipReleaseImage(srcImg);
  gEffect->clipReleaseImage(dstImg);
  return ok ? kOfxStatOK : kOfxStatFailed;
}
```

Leave the existing CPU `processRGBA(...)` call immediately after that block.

- [ ] **Step 5: Restore GitHub Metal build steps**

In `scripts/build_macos.sh`, restore tool checks and metallib packaging:

```bash
xcrun --sdk macosx --find metal >/dev/null
xcrun --sdk macosx --find metallib >/dev/null

echo "== Compile Metal shader =="
xcrun --sdk macosx metal -O3 -c shaders/PresenceKernels.metal -o build/PresenceKernels.air
test -s build/PresenceKernels.air

echo "== Link Metal library =="
xcrun --sdk macosx metallib build/PresenceKernels.air -o build/PresenceKernels.metallib
test -s build/PresenceKernels.metallib

mkdir -p "$BUNDLE/Contents/Resources"
cp build/PresenceKernels.metallib "$BUNDLE/Contents/Resources/PresenceKernels.metallib"
test -s "$BUNDLE/Contents/Resources/PresenceKernels.metallib"
```

In `.github/workflows/build.yml`, restore:

```yaml
xcrun --find metal
xcrun --find metallib
```

- [ ] **Step 6: Update sanity checks**

Update `ci/source_sanity.py` so it requires:

```python
'Metal declared':'kOfxImageEffectPropMetalRenderSupported' in cpp,
'Metal enabled gate':'kOfxImageEffectPropMetalEnabled' in cpp,
'Direct MTLBuffer':'id<MTLBuffer> inB' in mm and 'id<MTLBuffer> outB' in mm,
'CMake Metal bridge':'MetalBridge.mm' in cm and 'OBJCXX' in cm,
'Metal SDK compile':'xcrun --sdk macosx metal' in bs and 'xcrun --sdk macosx metallib' in bs,
```

Keep the export, universal, and tag release checks.

Update `ci/validate_bundle_macos.sh` to require:

```bash
test -s "$BUNDLE/Contents/Resources/PresenceKernels.metallib"
```

- [ ] **Step 7: Run validation**

Run:

```bash
python3 ci/source_sanity.py
g++ -std=c++17 -O2 -Wall -Wextra -Werror=return-type -Isrc tests/presence_model_tests.cpp src/PresenceCPU.cpp -o presence_model_tests && ./presence_model_tests && rm presence_model_tests
bash scripts/build_macos.sh
bash ci/validate_bundle_macos.sh build/PresenceOFX.ofx.bundle
```

Expected: all commands exit `0`; source sanity includes Metal gate checks; bundle contains `PresenceKernels.metallib`.

- [ ] **Step 8: Commit**

Run:

```bash
git add src/PresenceOFX_OFX.h src/PresenceOFX.cpp CMakeLists.txt scripts/build_macos.sh .github/workflows/build.yml ci/source_sanity.py ci/validate_bundle_macos.sh
git commit -m "Restore gated Metal render path"
```

### Task 2: Publish Release And Install GitHub Artifact

**Files:**
- Modify: `RELEASE_NOTES.md`
- Runtime install: `/Library/OFX/Plugins/PresenceOFX.ofx.bundle`

**Interfaces:**
- Consumes: committed Metal-enabled source from Task 1.
- Produces: GitHub release artifact installed in Resolve system OFX folder.

- [ ] **Step 1: Update release notes**

Append to `RELEASE_NOTES.md`:

```markdown
## v1.0.7 Metal render restore
- Restored OpenFX 1.5 Metal rendering behind the `OfxImageEffectPropMetalEnabled` gate.
- Kept CPU fallback for non-Metal render actions.
```

- [ ] **Step 2: Commit release notes**

Run:

```bash
git add RELEASE_NOTES.md
git commit -m "Update Metal release notes"
```

- [ ] **Step 3: Push and tag**

Run:

```bash
git push origin main
git tag v1.0.7
git push origin v1.0.7
```

- [ ] **Step 4: Wait for GitHub Actions**

Run:

```bash
gh run list --repo wbrisenold/PresenceOFX --limit 5
gh run watch --repo wbrisenold/PresenceOFX --exit-status
```

Expected: macOS build succeeds and publishes `PresenceOFX-macOS-universal.zip`.

- [ ] **Step 5: Download release artifact**

Run:

```bash
rm -rf /tmp/PresenceOFX-release
mkdir -p /tmp/PresenceOFX-release
gh release download v1.0.7 --repo wbrisenold/PresenceOFX --pattern 'PresenceOFX-macOS-universal.zip' --dir /tmp/PresenceOFX-release
ditto -x -k /tmp/PresenceOFX-release/PresenceOFX-macOS-universal.zip /tmp/PresenceOFX-release
```

Expected: `/tmp/PresenceOFX-release/PresenceOFX.ofx.bundle` exists.

- [ ] **Step 6: Install into Resolve OFX folder**

Run:

```bash
osascript -e 'do shell script "ditto /tmp/PresenceOFX-release/PresenceOFX.ofx.bundle /Library/OFX/Plugins/PresenceOFX.ofx.bundle && xattr -dr com.apple.quarantine /Library/OFX/Plugins/PresenceOFX.ofx.bundle" with administrator privileges'
codesign --verify --deep --strict /Library/OFX/Plugins/PresenceOFX.ofx.bundle
```

Expected: codesign verification exits `0`.

- [ ] **Step 7: Restart Resolve and verify scan**

Run:

```bash
osascript -e 'tell application id "com.blackmagic-design.DaVinciResolve" to quit' || true
sleep 8
open -a "DaVinci Resolve"
sleep 45
pgrep -fl "DaVinci Resolve|Resolve"
grep -n "com.luma.presenceofx\|Failed to load /Library/OFX/Plugins/PresenceOFX" "$HOME/Library/Application Support/Blackmagic Design/DaVinci Resolve/logs/davinci_resolve.log"
```

Expected: Resolve process exists; no new `Failed to load /Library/OFX/Plugins/PresenceOFX` after install.

## Self-Review

- Spec coverage: Metal gate, CPU fallback, GitHub Xcode build, release download, install, Resolve startup validation are covered.
- Placeholder scan: no placeholders remain.
- Type consistency: Metal gate property names match OpenFX docs and `PresenceOFX_OFX.h` additions.
