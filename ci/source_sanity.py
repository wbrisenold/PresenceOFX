from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
cpp=(r/'src/PresenceOFX.cpp').read_text(); bs=(r/'scripts/build_macos.sh').read_text(); cm=(r/'CMakeLists.txt').read_text(); wf=(r/'.github/workflows/build.yml').read_text()
checks={'OFX exports':all(x in cpp for x in ['OfxGetNumberOfPlugins','OfxGetPlugin','OfxSetHost']),'CPU OFX render':'processRGBA(' in cpp,'No Metal OFX path':'kOfxImageEffectPropMetalRenderSupported' not in cpp and 'kOfxImageEffectPropMetalCommandQueue' not in cpp and 'MetalBridge.mm' not in cm,'No Metal build step':'xcrun --sdk macosx metal' not in bs and 'metallib' not in bs,'Universal':'arm64;x86_64' in cm,'Tag release':'softprops/action-gh-release' in wf}
for k,v in checks.items(): print(f"[{'PASS' if v else 'FAIL'}] {k}")
if not all(checks.values()): sys.exit(1)
