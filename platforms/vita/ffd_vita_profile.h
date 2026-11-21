/* Source: uploader-labelled 1.3.0 ELF32 ARM soft-float libjniproxy.so.
 * SHA256 37a5e9fb3b5038f662f8ed67bca9004a4af9d7b540b98309931066a33d30a79f
 * This is game-binary compatibility metadata, not the proprietary binary.
 * Integration: include after ffd-vita/loader/so_util.h.
 */
#ifndef FFD_VITA_PROFILE_H
#define FFD_VITA_PROFILE_H
#include <stdint.h>
#include <string.h>
typedef struct {
    const char *symbol;
    uint32_t thumb_entry;
    uint32_t got_slot;
} FfdVitaSite;
enum {
    FFD_VITA_KEY_STATE, FFD_VITA_KEY_TRIGGER, FFD_VITA_KEY_REPEAT,
    FFD_VITA_MENU_UPDATE, FFD_VITA_MENU_DRAW,
    FFD_VITA_MAIN_VIEWPORT, FFD_VITA_GFX_VIEWPORT, FFD_VITA_GFX_2D_MATRIX,
    FFD_VITA_FIELD_DRAW, FFD_VITA_FIELD_FAR, FFD_VITA_FIELD_MINIMAP,
    FFD_VITA_FIELD_MESSAGE, FFD_VITA_GFX_DEST_WIDTH, FFD_VITA_GFX_DEST_HEIGHT,
    FFD_VITA_GET_TOUCH_PANEL, FFD_VITA_SITE_COUNT
};
static const FfdVitaSite ffd_vita_sites[FFD_VITA_SITE_COUNT] = {
    {"_ZN9GameClass8KeyStateEi", 0x000ca6a5u, 0x0028ad24u},
    {"_ZN9GameClass10KeyTriggerEi", 0x000bf4b5u, 0x0028a288u},
    {"_ZN9GameClass9KeyRepeatEi", 0x000ca6bdu, 0x0028a32cu},
    {"_ZN10MenuWidget6UpdateEv", 0x000e90edu, 0x0028a2d8u},
    {"_ZN10MenuWidget4DrawER13MtxGraphics3D", 0x000e93afu, 0x0028a3a4u},
    {"_ZN9MainScene11SetViewPortER13MtxGraphics3D", 0x000d1b25u, 0x0028b86cu},
    {"_ZN13MtxGraphics3D11SetViewPortEiiii", 0x00071e2du, 0x0028b85cu},
    {"_ZN13MtxGraphics3D15Set2DModeMatrixEff", 0x00072a99u, 0x00289f8cu},
    {"_ZN10FieldClass12DrawMapFieldEP13MtxGraphics3Df", 0x000a9ffdu, 0x0028ad40u},
    {"_ZN10FieldClass7DrawFarEP13MtxGraphics3D", 0x000b1661u, 0x0028af64u},
    {"_ZN10FieldClass11DrawMiniMapEP13MtxGraphics3D", 0x000b27adu, 0x0028ad48u},
    {"_ZN10FieldClass11DrawMessageEP13MtxGraphics3D", 0x000ae8d7u, 0x0028ad5cu},
    {"_ZN13MtxGraphics3D19GetDestinationWidthEv", 0x00071c95u, 0x0028aa0cu},
    {"_ZN13MtxGraphics3D20GetDestinationHeightEv", 0x00071cabu, 0x0028aa10u},
    {"_Z12GetTouchInfov", 0x0007c341u, 0x0028a2a0u},
};
static int ffd_vita_validate_site(so_module *mod, unsigned index)
{
    if (!mod || index >= FFD_VITA_SITE_COUNT || !mod->dynsym ||
        !mod->relplt || !mod->dynstr || !mod->text_base ||
        mod->num_relplt <= 0 || mod->num_relplt > 100000 ||
        mod->num_dynsym <= 0 || mod->num_dynsym > 100000) return 0;
    const FfdVitaSite *site = &ffd_vita_sites[index];
    if (so_symbol(mod, site->symbol) != mod->text_base + site->thumb_entry) return 0;
    int matches = 0;
    for (int i = 0; i < mod->num_relplt; ++i) {
        Elf32_Rel rel = mod->relplt[i];
        uint32_t sym_index = ELF32_R_SYM(rel.r_info);
        if (rel.r_offset != site->got_slot ||
            ELF32_R_TYPE(rel.r_info) != R_ARM_JUMP_SLOT ||
            sym_index >= (uint32_t)mod->num_dynsym) continue;
        const char *name = mod->dynstr + mod->dynsym[sym_index].st_name;
        if (strcmp(name, site->symbol) == 0) matches++;
    }
    if (matches != 1) return 0;
    return *(const volatile uint32_t *)(mod->text_base + site->got_slot) ==
           (uint32_t)(mod->text_base + site->thumb_entry);
}
static int ffd_vita_validate_group(so_module *mod, const unsigned *sites, unsigned n)
{
    if (!sites || !n) return 0;
    for (unsigned i = 0; i < n; ++i)
        if (!ffd_vita_validate_site(mod, sites[i])) return 0;
    return 1;
}
static void ffd_vita_replace_slot(so_module *mod, unsigned site_index, uintptr_t replacement)
{
    *(volatile uint32_t *)(mod->text_base + ffd_vita_sites[site_index].got_slot) =
       (uint32_t)replacement;
}
#endif
