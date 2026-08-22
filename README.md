# PresenceOFX

PresenceOFX is the **make the picture feel better** stage in the KB Tools
workflow for DaVinci Resolve Studio. It can add local shape, gentle detail,
soft edges, atmosphere, highlight presence, shadow presence, texture, and small
optical bloom.

It is not a camera translator, not an exposure tool, not a LUT, and not a film
emulation plugin.

## If you are new

Start with the [KB Tools Beginner Handbook](https://wbrisenold.github.io/KB-Tools/guides/beginner-handbook.html). It begins with Blackmagic Camera on a phone, explains Log and exposure, and walks through a complete first grade.

The larger order is:

```text
Camera clip
    -> CST: translate the camera file
    -> PresenceOFX: shape the feeling
    -> Keystone: fix balance and tone
    -> Color Separation: choose the color relationship
    -> Referent ODT: always-on viewing transform
    -> Look LUT
    -> MonoNodes QC
```

**Analogy:** PresenceOFX is like changing hard ceiling light into softer window
light. It can make the picture more comfortable, but it cannot tell the camera
what language it recorded or fix a picture that is too dark.

![Resolve node editor](https://wbrisenold.github.io/KB-Tools/assets/images/resolve/nodes.jpg)

## What you need before adding PresenceOFX

- DaVinci Resolve Studio.
- A camera or phone clip.
- A CST before PresenceOFX.
- The real camera Log format, if the clip was recorded in Log.

The source may be Apple Log, Canon Log 3, ARRI LogC3, or another format. Apple
Log is not the same as Canon Log 3. The CST handles that difference before this
plugin sees the picture.

## How to install the plugin

This repository can build a universal macOS OFX bundle through GitHub Actions.
The output is:

```text
PresenceOFX.ofx.bundle
```

Install it in the system Resolve folder:

```text
/Library/OFX/Plugins/
```

The included `scripts/install_macos.sh` uses administrator permission for that
folder. Restart Resolve after installing. Do not leave a second copy in a
user-level OFX folder while troubleshooting discovery.

## How to add PresenceOFX in Resolve

1. Open the Color page.
2. Select the node after the CST.
3. Open Effects Library in the upper-right.
4. Search for **PresenceOFX**.
5. Add it to that node.
6. Keep Amount low while you learn.
7. Change one control, then bypass the PresenceOFX node to compare.
8. Keep Referent enabled later in the chain because it is the ODT/viewing transform.

## Controls in everyday language

- **Amount:** how much of the whole effect you want.
- **Depth:** broad shape and separation between parts of the picture.
- **Micro:** small detail and crispness.
- **Atmosphere:** less haze at negative values, more air or veil at positive values.
- **Edge Soft:** takes the brittle edge off digital detail.
- **Hi Presence:** adds shape to bright surfaces.
- **Sh Presence:** adds shape to darker areas without simply lifting them.
- **Texture:** adds a little surface feeling. It is not film grain.
- **Bloom:** lets bright areas blend gently into nearby areas.
- **Skin Guard:** makes the effect gentler on likely skin colors.
- **View:** shows the normal result, the effect mask, the edge mask, or the difference.

## What to do first

1. Turn PresenceOFX on with Amount low.
2. If the image feels flat, try a little Depth.
3. If it feels soft, try a little Micro.
4. If it feels too digital, lower Micro or use Edge Soft.
5. If it looks foggy, lower Atmosphere or Bloom.
6. If faces become hard, use Skin Guard and reduce the amount.
7. Compare with the node bypassed at the same viewing transform.

## Referent stays on

Referent is Cullen Kelly's free viewing LUT and display foundation. In this
workflow it is the ODT. Keep it enabled while you judge PresenceOFX and while
you grade. It is not a look to turn off and on for a normal creative
comparison.

You may bypass Referent briefly to inspect the underlying LogC3 signal during
troubleshooting, then turn it back on before making a decision.

## If the result is wrong

- The image is pale and gray: check the CST. PresenceOFX is not the viewing transform.
- The image is crispy: lower Micro or Amount, then try Edge Soft.
- The image is foggy: lower Atmosphere or Bloom.
- Faces look too hard: use Skin Guard and reduce Micro or Hi Presence.
- Shadows look crunchy: reduce Sh Presence or Depth.
- You cannot tell what changed: use the View modes and bypass the node.
- Resolve cannot find the plugin: install it in `/Library/OFX/Plugins/` and restart Resolve.

## Build and validation

This project uses a minimal OpenFX setup and does not require users to install
Xcode or Homebrew locally. GitHub Actions builds the macOS bundle and checks the
bundle exports. Local source checks are available in `ci/source_sanity.py`.

The real release gate is opening the plugin in Resolve and applying it to
representative footage without a crash. The connected workflow was tested on
Apple Log and Canon Log 3 footage, but that does not guarantee every phone,
camera, GPU, Resolve version, or monitoring pipeline.

## Learning resources and handoff

## Fifteen visual lessons for PresenceOFX users

Use these cards to learn what belongs before and after PresenceOFX. The plugin
is easier to use when you can recognize the rest of the chain:

1. [Download Blackmagic Camera](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#phone-download)
2. [Set up the phone](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#phone-settings)
3. [Understand a pale Log clip](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#log-recording)
4. [Find the Color page](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#color-page)
5. [Read the node editor](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#nodes)
6. [Check the CST before PresenceOFX](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#cst)
7. [Use primary controls](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#primary)
8. [Place PresenceOFX](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#presence)
9. [Hand off to Keystone](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#keystone)
10. [Keep the Referent ODT on](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#referent)
11. [Keep the Look LUT after Referent](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#look)
12. [Use waveform](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#waveform)
13. [Use color scopes](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#color-scopes)
14. [Use qualifier, window, and tracker](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#selections)
15. [Compare before and after](https://wbrisenold.github.io/KB-Tools/guides/resolve-resource-library.html#compare)

Each card has one picture, one beginner explanation, and one authoritative
source link. Read the card that matches the problem you are seeing instead of
changing several PresenceOFX controls at once.

- [KB Tools Beginner Handbook](https://wbrisenold.github.io/KB-Tools/guides/beginner-handbook.html)
- [KB Tools Resolve Visual Atlas](https://wbrisenold.github.io/KB-Tools/guides/resolve-visual-atlas.html)
- [Blackmagic Design Color training](https://www.blackmagicdesign.com/products/davinciresolve/training)
- [Cullen Kelly Color](https://www.youtube.com/@CullenKelly)
- [Cullen Kelly Referent](https://cullenkellycolor.com/toolkit/referent)
- [Keystone](https://github.com/wbrisenold/Keystone)

## Credits and license

PresenceOFX is distributed under the repository license. The camera and color
reference images used in the teaching site are credited in the KB Tools visual
references. This repository's plugin is separate from Referent, Keystone, Henry
Bobeck's Color Separation, and MonoNodes.
