# PresenceOFX

**PresenceOFX** is the image-character stage in a small DaVinci Resolve system. It sits after the camera transform and before Keystone, giving technically balanced footage more dimensionality without turning the grade into a stack of disconnected effects.

It is not a film-emulation plugin and it is not a LUT. It shapes the image spatially through local tonal separation, frequency shaping, edge-safe presence, highlight/shadow detail, subtle optical bloom, and texture response. The output stays in the working signal so Keystone can perform the technical balance that follows.

## Core controls

- **Amount** — global mix for the whole effect.
- **Depth** — broad local contrast / subject separation.
- **Micro** — mid-frequency detail contrast.
- **Atmosphere** — negative values dehaze; positive values add air/veil.
- **Edge Soft** — reduces brittle digital edges after presence is added.
- **Hi Presence** — detail/shape in bright surfaces.
- **Sh Presence** — local separation in dark areas without simply lifting blacks.
- **Texture** — subtle surface-density response.
- **Bloom** — small optical highlight integration.
- **Skin Guard** — reduces harsh presence on likely skin colors.
- **View** — Normal, Presence Mask, Edge Mask, Difference.

## Suggested node placement

Typical pre-ODT grading stack:

```text
CST: camera -> AWG3 / LogC3
    -> PresenceOFX
    -> Keystone
    -> HB Color Separation DCTL
    -> [Referent ODT](https://cullenkellycolor.com/toolkit/referent): LogC3 -> display space
    -> Look LUT: display-referred look
    -> [MonoNodes Chart DCTL](https://mononodes.com/dctls/): final chart / display QC
```

The connected system keeps PresenceOFX and Keystone in the LogC3 working space. After Keystone, Henry Bobeck's paid [Color Separation DCTL](https://henrybobeck.com/dctl/ColorSeparation) provides a separate creative separation stage. [Referent](https://cullenkellycolor.com/toolkit/referent) is Cullen Kelly's free viewing LUT and display foundation, while [MonoNodes](https://mononodes.com/dctls/) provides DCTL and workflow tools for the chart/QC end of the chain. Purchase the paid separation tool from Henry if you use it; none of these companion tools are part of this repository.

For display-referred finishing, use less Amount and less Micro.

## GitHub build workflow

This repo is built for users who do **not** have Xcode locally.

Push the repo to GitHub, then push a tag such as:

```bash
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions will build a universal macOS OFX bundle and publish a release ZIP.

## Architecture

This follows the proven no-SDK/no-Homebrew pattern from our earlier OFX work:

- no external OpenFX SDK checkout
- no Homebrew packages
- no `expat::expat`
- pinned minimal OFX ABI header in `src/PresenceOFX_OFX.h`
- CPU OpenFX renderer
- universal `arm64` + `x86_64` binary
- ad-hoc signed bundle
- export-symbol checks for `OfxGetPlugin`, `OfxGetNumberOfPlugins`, and `OfxSetHost`
- release ZIP produced automatically from tags

## Install location

The build output is:

```text
PresenceOFX.ofx.bundle
```

Install to:

```text
/Library/OFX/Plugins/
```

or user-local:

```text
~/Library/OFX/Plugins/
```

Restart Resolve after installing.

## Production status

This is a production-candidate repo package. The CPU algorithm and test harness are included, and GitHub Actions performs binary validation. The true release gate is Resolve runtime testing on representative footage.

## AI-assisted development and testing

This project was vibe coded with human direction and AI assistance, then organized around explicit source code, tests, ABI checks, and GitHub Actions validation. The connected workflow was tested on Apple Log and Canon Log 3 footage. Those tests reflect the author's setup and do not replace a host test on your Resolve version, GPU, and media.
