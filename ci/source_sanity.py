from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
cpp=(r/'src/PresenceOFX.cpp').read_text(); mm=(r/'src/MetalBridge.mm').read_text(); bs=(r/'scripts/build_macos.sh').read_text(); cm=(r/'CMakeLists.txt').read_text(); wf=(r/'.github/workflows/build.yml').read_text()
checks={'OFX exports':all(x in cpp for x in ['OfxGetNumberOfPlugins','OfxGetPlugin','OfxSetHost']),'Metal declared':'kOfxImageEffectPropMetalRenderSupported' in cpp,'Direct MTLBuffer':'id<MTLBuffer> inB' in mm and 'id<MTLBuffer> outB' in mm,'No unsafe copy':'newBufferWithBytes:src' not in mm and 'memcpy(dst' not in mm,'dladdr metallib':'dladdr' in mm,'xcrun tools':'xcrun --sdk macosx --find metal' in bs and 'xcrun --sdk macosx metal' in bs,
'Metal SDK compile':'xcrun --sdk macosx metal' in bs and '-std=metal' not in bs and '-std=macos-metal' not in bs,'Universal':'arm64;x86_64' in cm,'Tag release':'softprops/action-gh-release' in wf}
for k,v in checks.items(): print(f"[{'PASS' if v else 'FAIL'}] {k}")
if not all(checks.values()): sys.exit(1)
