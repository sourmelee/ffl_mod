# FFD Portable Controls + True Widescreen (development prototype)

**Scope:** Only game-aware controller navigation and expanded field rendering. No gameplay patches, translations, job changes, save changes, or game asset redistribution.

**Status:** Portable C logic has host tests; neither console's proprietary game binary has been supplied or reverse-engineered here, so **no Vita or Switch runtime hooks or playable builds exist yet**. The existing 1.1.9 Switch and 1.1.3 Vita binaries are intentionally retained.

## Files

- [portable/ffd_nav.h](../portable/ffd_nav.h): reusable geometric directional candidate selection with row/column preference, wraparound, item identity persistence, selection-by-widget and logical touch output. Adapted from the existing FFLmod `nav()` logic.
- [portable/ffd_display_transform.h](../portable/ffd_display_transform.h): logical-to-physical coordinate mapping using the existing [display_geometry.h](../src/display_geometry.h).
- [tests/portable_port_test.c](../tests/portable_port_test.c): test coverage for 960x544 Vita, 1280x720/1920x1080 Switch, 4:3, 16:10, grid movement, wrapping, selection persistence and activation.
- A standalone ELF symbol/relocation scanner and identical portable sources are available in the accompanying development archive.

## Host test

From the repository root:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic tests/portable_port_test.c -lm -o /tmp/ffd-port-test
/tmp/ffd-port-test
```

## Switch (ffd_nx, ARM64, Android game version 1.1.9)

1. Inspect the user's legally obtained 1.1.9 `libjniproxy.so`; create a guarded hash/profile. **Never assume 1.2.0 addresses match 1.1.9.**
2. Identify `GameClass::KeyState/KeyTrigger/KeyRepeat`, `MenuWidget::Update/Draw`, touch-panel access, and graphics/field projection hooks by symbols and disassembly. Validate original functions and relocation slots against the exact executable image.
3. Implement an optional patch installer in `source/main.c` after `ffd_resolve_imports(&game_mod)` and before `so_finalize(&game_mod)`; abort patch installation without affecting the original game if required hooks are missing.
4. After original `MenuWidget::Update`, enumerate live/active/visible/touchable items into `FfdNavCandidate[]`. Feed debounced directions to `ffd_nav_step()` only if the game has not already processed a native key; translate its output into correctly timed engine touch events. Preserve physical touch and existing fallback controls.
5. Adapt FFLmod's `MainScene::SetViewPort`, `Set2DModeMatrix`, `FieldClass::DrawMapField`, far-background, fill, minimap and widget-position hooks for 1.1.9. `display_geometry()` gives the layout, **but expanded map camera/tile culling needs real engine hooks**.
6. Add separate off-by-default `enhanced_navigation` and `expanded_field` settings and runtime diagnostics. Avoid in-place ARM64 function hooks without a verified trampoline, or prefer validated GOT interception.

Existing port infrastructure: `source/so_util.c` has ARM64 symbol lookup and `hook_arm64`; `source/main.c` already sends simulated touches. Do not directly call overwritten prologues after function interception.

## Vita (ffd-vita, ARMv6, Android game version 1.1.3)

1. Independently inspect the 32-bit 1.1.3 `libjniproxy.so` and validate function signatures/layouts.
2. Use `loader/main.c::patch_game()` after the existing relocation and symbol resolver, with ARMv6/Thumb-appropriate branch/trampoline handling. **Do not transplant ARM64 patch instructions.**
3. Reuse the same portable candidate algorithm/display geometry, but build Vita-specific menu enumeration, input adapters and widescreen hooks. Test Vita's 960x544 viewport and limited GPU/memory resources.
4. Preserve original touch input and drawing when any required patch site fails validation.

## Why binary versions still matter

The *behavior* of these two features is reusable. Their integration points—native functions, GOT relocation addresses, object memory layouts, and field rendering internals—depend on the exact binary and architecture. Missing symbol names in a stripped executable require careful reverse engineering, not guessed offsets.

## Quality gate for a console release

Do not release an NRO/VPK until a device test demonstrates: original gameplay with features disabled; correct field controls; battle/menu cursor; save/load; original and expanded projection; dialogue, minimap, culling, overlays and touch-coordinate alignment; safe fallback if the game executable does not match the profile.

The code is MIT-licensed and contains no copyrighted game files.
