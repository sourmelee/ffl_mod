# Vita ARM32 compatibility profile (1.1.3)

## Install the experimental VPK and launch it

The end-user instructions (including VitaShell installation, Vita LiveArea launch, kernel plugins, libshacccg, legally owned game files, configuration, backup and rollback) are in **[Portable console installation guide](../../PORTABLE_CONSOLE_INSTALLATION.md#playstation-vita--installation-and-launch)**. The original upstream author is [Rinnegatamante](https://github.com/Rinnegatamante/ffd-vita). The compiled preview VPK is on the [experimental release](https://github.com/sourmelee/ffl_mod/releases/tag/console-preview-2026-11-21).

## Original port authors and licenses

**This is an experimental enhancement to [Rinnegatamante's original PlayStation Vita port, ffd-vita](https://github.com/Rinnegatamante/ffd-vita), not a new Vita port.** The original Vita loader, application/platform integration, graphics, input and VPK packaging are his original project. Controller and widescreen ideas were adapted from the **[upstream Android fflmod/ffl_mod](https://github.com/fflmod/ffl_mod)**.

The original Vita README additionally credits **[TheOfficialFloW](https://github.com/TheOfficialFloW)** (TheFloW) for the shared-library loader, **[frangarcj's ff5_vita](https://github.com/frangarcj/ff5_vita)** for code reused by the original port, **withLogic** and **Hans** for testing, and **Brandonheat8** for the original LiveArea graphics. The original Vita MIT license names **Andy Nguyen** as copyright holder.

See [complete attribution and source links](../../PORTABLE_CONSOLE_CREDITS.md) and [copyright and license notices](../../PORTABLE_CONSOLE_NOTICES.txt). The upstream authors are not implied to have endorsed this experimental build.

This directory records read-only analysis of the user's own library, not a modified or redistributed game binary.

- ELF32 / ARMv5TE Thumb-1 / soft-float EABI5. Compatible with Vita's ARMv7 ISA.
- SHA-256: 37a5e9fb3b5038f662f8ed67bca9004a4af9d7b540b98309931066a33d30a79f
- GNU Build ID: 071d80506d9f9c5fa03f7a7eb6964994a58b2f48
- Version designation: 1.1.3, as specified for the Vita port. The ELF build identity and SHA-256 remain authoritative for binary-hook compatibility.
- The original Rinnegatamante/ffd-vita README documents the 1.1.3 game version. Basic startup with the uploaded binary has not been verified.

The 15 native functions and their R_ARM_JUMP_SLOT positions are in [vita_113_profile.json](vita_113_profile.json). [ffd_vita_profile.h](ffd_vita_profile.h) checks exported symbols, relocation ownership and unmodified GOT values before loader hooks. Actual Vita adapter code is experimental and needs hardware testing.

**Navigation:** MenuWidget::Draw uses a linked-list head at widget+0x10; node+0x8 points to MenuItem; ordinary item geometry begins at item+0x1c according to ARM Thumb disassembly. Specialized battle/job list subitems still need inspection.

**Widescreen:** MtxGraphics3D::SetViewPort stores x/y/width/height as floats at gfx+0x4ac, +0x4b0, +0x4b4, +0x4b8. Full expanded field still needs projection, camera/culling, backgrounds, HUD/dialogue, minimap, touch/scissor alignment, and handling of the original Vita loader's borders.png overlay after every frame. Expanding glViewport alone is not sufficient.

The original game shared library is not modified or committed. The experimental Vita VPK **successfully cross-compiled** with the VitaSDK softfp Docker image (GitHub Actions [build 44945275143](https://github.com/sourmelee/ffl_mod/actions/runs/44945275143)), but **boot, navigation, widescreen and save behavior remain unverified on real Vita hardware**. The compiled VPK is not a stable release.
