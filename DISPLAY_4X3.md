# Native 4:3 display support

The supplied Nova screenshot, `screenshots/campfire.jpg`, is 1536x1152 (4:3).
It shows the original approximately 5:3 game area with decorative bands above
and below. The existing mod expanded only horizontal space on wider screens.

The source now automatically supports a taller landscape canvas, including
4:3, while retaining the existing widescreen path. There is no new setting to
toggle: it follows the current drawing surface. Portrait/square surfaces keep
the game's original layout.

| Surface | Logical canvas | Added visible space |
| --- | --- | --- |
| 1536x1152, 4:3 | 640x480 | 48 units above and below |
| 1920x1200, 16:10 | 640x400 | 8 units above and below |
| 1600x960, original 5:3 | 640x384 | None |
| 1920x1080, 16:9 | About 682.67x384 | Existing extra width |

For 4:3 this adds 25% vertical world space at the same uniform pixel scale.
Sprites and text retain their proportions; backgrounds and full-screen fills
extend to cover the additional space. The field renderer's native height-based
camera and tile culling use the expanded canvas. Maps still have their original
boundaries; this change does not invent map content outside them.

## Integration

- The viewport uses the actual destination size, avoiding integer letterbox
  rounding that can leave a border on the Nova's 1536x1152 surface.
- Touch conversion, font projection, clipping, and controller-generated taps
  share the same uniform scale and logical origin. OpenGL clipping has the
  opposite Y-origin adjustment from Android touch coordinates.
- Field buttons and minimap move toward the top edge. The movement receiver
  expands to cover the taller view and is restored after update/draw.
- Top/bottom dialogue stays at its chosen edge. Choice widgets move during
  both update and drawing so their hit areas match their visible positions.
  Other menus retain their centered layout.
- Backgrounds, fades, and screen masks cover the new height. Normal sprites,
  rotated images, square subviews, and render textures retain their geometry.
- Field dimensions are corrected before map rendering, avoiding the original
  float-to-integer calculation truncating a 480-unit canvas to 479.
- New destination queries and field/message hooks resolve by symbol with the
  existing compatibility checks. If the required taller-view hooks are missing,
  the game keeps its original letterboxed layout.

The display code was moved from `src/fflmod.c` into `src/display_runtime.h` so
the actual rendering hooks can be tested with mock graphics/touch memory.
Shared geometry is in `src/display_geometry.h`.

## Verification and remaining limits

Host tests cover 11 resolutions, portrait/square fallback, 4:3/16:9 transitions,
uniform sprite scale, destination rounding, touch/scissor alignment, background
and sprite handling, dialogue and HUD alignment, restoration, render textures,
and the required-hook fallback. ARM64 C compile/static analysis also passed.

These are source and simulated-memory checks, not Android screenshots or a
playthrough. The existing environment blockers remain: no complete native
compiler/smali toolchain can be downloaded, and ADB cannot connect. No updated
`.so`/DEX game payload has been built or installed. The manager APK alone does
not contain this rendering change. The added CI display test runs alongside
the translation tests when the source is eventually pushed; no remote workflow
has been run or release published.

On a build host, run the two host tests and the normal source-only payload build:

```sh
mkdir -p out
clang -std=c11 -Wno-unused-function tests/localization_test.c -o out/localization-test
out/localization-test
clang -std=c11 -Wno-unused-function tests/display_test.c -o out/display-test
out/display-test
APKTOOL=/path/to/apktool_2.10.0.jar sh tools/build.sh - out/native
python3 tools/mkpatch.py out/native/work out/payload 1.1.15-en1
```

On the Nova, verify the campfire scene, small-map boundaries, world map/airship,
battles and battle controls, menus, top/bottom dialogue and choices, minimap,
touch/controller input, fades, encounter transitions, lighting, and save screens.
Repeat key checks on a 16:9 surface before distributing a release.
