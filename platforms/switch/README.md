# Switch portable enhancements — uploaded ARM64 candidate for game version 1.1.9

**Status: source-only experimental controller adapter, not a released NRO.** The user's library is an Android ELF64 AArch64 shared object with many native engine exports. The supplied version is believed to be 1.1.9, but **version 1.1.9 is not independently encoded or verified in the ELF**. Compatibility is instead pinned to the full binary hash.

- SHA-256: `4c6dec4cec9cb58dddcb85a7dd70e289356d55564902949da2a21844b38ef93e`
- GNU build ID: `ed387bde786c58d236de2ac14f6953c224326d34`
- 19 symbol locations and relocation slots: [switch_119_profile.json](switch_119_profile.json)
- Conservative before-hook checks: [ffd_switch_profile.h](ffd_switch_profile.h)
- Confirmed object layout and repeat logic: [ffd_switch_menu_layout.h](ffd_switch_menu_layout.h)
- Opt-in menu hook bridge: [ffd_switch_nav_experiment.h](ffd_switch_nav_experiment.h)
- Reversible source-level installer: [apply_switch_loader.py](apply_switch_loader.py)

## Locally verify the owned binary

```sh
python3 platforms/switch/verify_switch_profile.py '/path/to/libjniproxy.so'
```

The validator checks SHA-256, ELF class/architecture, **all 19** native symbol addresses and their `R_AARCH64_JUMP_SLOT` locations using `readelf`.

## Apply to NaGaa95/ffd_nx

On a copy of your locally checked-out upstream project:

```sh
python3 platforms/switch/apply_switch_loader.py /path/to/ffd_nx --dry-run
python3 platforms/switch/apply_switch_loader.py /path/to/ffd_nx
```

The patch script copies the source-only helpers into the Switch checkout, saves `source/main.c.ffd-bak`, and modifies carefully checked source anchors. It does not copy or redistribute the game executable or assets. Inspect the resulting `git diff` before attempting a devkitPro/libnx build.

Create `enhanced.txt` next to `config.txt` containing:

```ini
game_aware_navigation=1
```

Without the file or when the flag is 0, no extra menu hook is installed. If the exact symbol/GOT profile fails at runtime, the feature stays disabled. **A matching full-file hash must also be confirmed offline**; the on-device bootstrap validates symbols and resolved GOT entries rather than hashing the game library.

### Controls for the initial experiment

- Hold ZL + D-pad or left stick to select visible, touchable ordinary MenuItems, with directional wrapping and key repeat.
- Hold ZL + A to send a touch at the selected item's centre.
- Ordinary field movement, real screen touches, B/BACK, and the port's current on-screen controls remain untouched when ZL is not used.
- The existing right-stick cursor remains available.

This intentionally avoids automatically replacing field controls. The selection is generated from actual `MenuWidget::Draw` linked lists, not from predetermined menu coordinates. ARM64 disassembly of the uploaded file confirms widget+0x20, node+0x10, item flags +0x0c/+0x0d/+0x14 and geometry +0x28..+0x34. Specialized job/battle subitem grids, choice-dialog state and screen-context detection need further integration.

## Widescreen work

This first Switch contribution profiles native `MainScene::SetViewPort`, `MtxGraphics3D::SetViewPort`, `Set2DModeMatrix`, `FieldClass::DrawMapField`, `DrawFar`, `DrawMiniMap`, `DrawMessage`, `DrawImageEx` and `FillRect` functions. The platform-neutral [display geometry](../../src/display_geometry.h) and [coordinate conversion](../../portable/ffd_display_transform.h) already cover 1280x720 and 1920x1080.

**True expanded field rendering is not installed.** Merely widening the viewport or changing a 2D projection does not guarantee correct tile culling, camera bounds, offscreen backgrounds, dialogue, HUD, battle effects, touch and scissor positioning. That entire group must be implemented and hardware-tested together before a widescreen toggle is offered.

## Safety and release gates

- The upstream `ffd_nx` loader is not changed on GitHub: this patch lives in a branch of `sourmelee/ffl_mod`, which contains the portable source.
- No NRO build, on-device runtime verification, scene smoke tests or performance profiling has been completed.
- Do not distribute the original game `.so` or `.obb`.
- This is a developmental experiment, not a completed gamepad/widescreen release.
