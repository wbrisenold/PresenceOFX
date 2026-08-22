# PresenceOFX

**PresenceOFX** is the image-character stage in a small DaVinci Resolve system. It sits after the camera transform and before Keystone, giving technically balanced footage more dimensionality without turning the grade into a stack of disconnected effects.

It is not a film-emulation plugin and it is not a LUT. It shapes the image spatially through local tonal separation, frequency shaping, edge-safe presence, highlight/shadow detail, subtle optical bloom, and texture response. The output stays in the working signal so Keystone can perform the technical balance that follows.

## PresenceOFX's role

PresenceOFX belongs early in the connected tree, after the camera signal has
been transformed to LogC3 and before Keystone performs the main balance. It is
for image structure and character: local separation, controlled detail,
edge behavior, atmosphere, and restrained optical integration. It is not the
place to solve exposure, display conversion, or a finished film look.

That distinction is useful in practice. If the image feels brittle or flat,
PresenceOFX is the stage to audition. If the image is technically wrong,
correct it in Keystone. If the final display foundation is wrong, inspect
[Referent](https://cullenkellycolor.com/toolkit/referent) after the LogC3 work.

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

## How it feels in a grade

PresenceOFX is meant to make a digital image feel more settled, not louder.
At sensible settings, edges lose their brittle quality, faces hold together,
and local contrast feels more like light shaping the subject than sharpening
being applied to the frame. The effect should be judged at the final viewing
transform, because a small change in a scene-referred signal can become much
more obvious after display conversion.

The controls are separated by the kind of response they create:

| Control | What you feel | Why the slider exists |
|---|---|---|
| Amount | The overall strength of the stage. | A single mix makes it easy to compare the complete character pass against bypass. |
| Depth | Broad subject separation and a sense of dimensional weight. | It changes local structure rather than simply adding global contrast. |
| Micro | Texture and mid-frequency articulation. | It adds definition where the image feels soft without making every edge sharper. |
| Atmosphere | Less haze at negative values, more air or veil at positive values. | It gives the image a broad environmental response instead of a local-only effect. |
| Edge Soft | Less brittle high-frequency transition. | It reins in digital acutance after presence has been added. |
| Hi Presence / Sh Presence | More shape in bright surfaces or dark areas. | Highlights and shadows often need different treatment; one global contrast control cannot separate them cleanly. |
| Texture | Subtle surface density. | It gives material a little more physical response without pretending to be film grain. |
| Bloom | Small highlight integration. | It connects bright areas to their surroundings without turning the image into a glow effect. |
| Skin Guard | A softer response on likely skin colors. | Presence that works on architecture or fabric can become harsh on faces. |
| View | A way to inspect the effect instead of judging it by taste alone. | The masks show whether a slider is acting on the region you intended. |

Start with Amount low, find the control that addresses the problem, then raise
that control before raising the whole effect. If the image starts to look
crispy, reduce Micro or add Edge Soft. If it starts to look washed out, reduce
Atmosphere or Bloom before changing Keystone's contrast.

## Suggested node placement

PresenceOFX is the first creative stage after the camera transform. The complete system map is maintained in the [KB Tools node guide](https://wbrisenold.github.io/KB-Tools/guides/resolve-node-guide.html).

```text
Camera transform -> LogC3
    -> PresenceOFX
    -> Keystone
    -> technical balance and later system stages
```

The connected system keeps PresenceOFX and Keystone in the LogC3 working space. After Keystone, Henry Bobeck's paid [Color Separation DCTL](https://henrybobeck.com/dctl/ColorSeparation) provides a separate creative separation stage. [Referent](https://cullenkellycolor.com/toolkit/referent) is Cullen Kelly's free viewing LUT and display foundation, while [MonoNodes](https://mononodes.com/dctls/) provides DCTL and workflow tools for the chart/QC end of the chain. Purchase the paid separation tool from Henry if you use it; none of these companion tools are part of this repository.

Read the [PresenceOFX section of the system guide](https://wbrisenold.github.io/KB-Tools/guides/resolve-node-guide.html#what-each-stage-is-doing) for the handoff into Keystone and the later display stages. This README stays focused on image character and control response.

For display-referred finishing, use less Amount and less Micro.

## A practical starting point

Begin with the effect bypassed and enable only the controls that answer the
problem in front of you. Use the diagnostic views before increasing Amount or
Micro. A little Depth, Micro, or Bloom can change the perceived sharpness of
the whole image, so compare against the bypass at the same display transform.

Keep Skin Guard available when the effect is used on faces, and use Edge Soft
to reduce brittle transitions rather than blurring the entire frame.

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
