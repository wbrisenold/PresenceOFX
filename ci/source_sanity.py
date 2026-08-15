from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
cpp=(r/'src/PresenceOFX.cpp').read_text(); mm=(r/'src/MetalBridge.mm').read_text(); bs=(r/'scripts/build_macos.sh').read_text(); cm=(r/'CMakeLists.txt').read_text(); wf=(r/'.github/workflows/build.yml').read_text(); hdr=(r/'src/PresenceOFX_OFX.h').read_text()
checks={'OFX exports':all(x in cpp for x in ['OfxGetNumberOfPlugins','OfxGetPlugin','OfxSetHost']),'Metal declared':'kOfxImageEffectPropMetalRenderSupported' in cpp,'Metal enabled gate':'kOfxImageEffectPropMetalEnabled' in cpp and 'kOfxImageEffectPropMetalEnabled' in hdr,'Direct MTLBuffer':'id<MTLBuffer> inB' in mm and 'id<MTLBuffer> outB' in mm,'CMake Metal bridge':'MetalBridge.mm' in cm and 'OBJCXX' in cm,'Metal SDK compile':'xcrun --sdk macosx metal' in bs and 'xcrun --sdk macosx metallib' in bs,'CPU fallback':'processRGBA(' in cpp,'Universal':'arm64;x86_64' in cm,'Tag release':'softprops/action-gh-release' in wf}
for k,v in checks.items(): print(f"[{'PASS' if v else 'FAIL'}] {k}")
if not all(checks.values()): sys.exit(1)
