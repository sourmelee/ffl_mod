# Vita ARM32 compatibility profile (uploaded binary labelled 1.3.0)

This directory records read-only analysis of the user's own library, not a modified or redistributed game binary.

- ELF32 / ARMv5TE Thumb-1 / soft-float EABI5. Compatible with Vita's ARMv7 ISA.
- SHA-256: 37a5e9fb3b5038f662f8ed67bca9004a4af9d7b540b98309931066a33d30a79f
- GNU Build ID: 071d80506d9f9c5fa03f7a7eb6964994a58b2f48
- The uploader's “1.3.0” version label is not independently confirmed by an embedded ELF release field.
- The original Rinnegatamante/ffd-vita README only documents game version 1.1.3 testing. Basic startup with this library has not been verified.

The 15 native functions and their R_ARM_JUMP_SLOT positions are in [vita_130_profile.json](vita_130_profile.json). [ffd_vita_profile.h](ffd_vita_profile.h) checks exported symbols, relocation ownership and unmodified GOT values before loader hooks. Actual Vita adapter code is experimental and needs hardware testing.

**Navigation:** MenuWidget::Draw uses a linked-list head at widget+0x10; node+0x8 points to MenuItem; ordinary item geometry begins at item+0x1c according to ARM Thumb disassembly. Specialized battle/job list subitems still need inspection.

**Widescreen:** MtxGraphics3D::SetViewPort stores x/y/width/height as floats at gfx+0x4ac, +0x4b0, +0x4b4, +0x4b8. Full expanded field still needs projection, camera/culling, backgrounds, HUD/dialogue, minimap, touch/scissor alignment, and handling of the original Vita loader's borders.png overlay after every frame. Expanding glViewport alone is not sufficient.

The original game shared library is not modified or committed. No Vita VPK build or gameplay verification has been performed.
