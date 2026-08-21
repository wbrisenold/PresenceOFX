# PresenceOFX

**PresenceOFX** is an OpenFX image-character plugin for DaVinci Resolve. It is designed for the problem where footage is technically balanced but still feels thin, cheap, hazy, or not fully present.

It is not a film-emulation plugin and it is not a LUT. It is a spatial image-character stage that adds dimensionality through local tonal separation, frequency shaping, edge-safe presence, highlight/shadow detail, subtle optical bloom, and texture response.

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
    -> PrimeraSkin
    -> HB Color Separation DCTL
    -> KH Gamut Compressor
    -> Referent LogC3 -> Rec.709 ODT LUT
    -> FilmBox Rec.709 look LUT
    -> MonoNodes Balance Charts
```

Advanced Toning and inactive LookLab WB are normally redundant in this tree.
Use either one only when a shot specifically needs its standalone behavior.

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
