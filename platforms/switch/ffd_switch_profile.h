/* For ffd_nx: include after source/so_util.h.
 * Exact symbol/GOT profile: uploaded AArch64 libjniproxy(2).so
 * SHA-256 4c6dec4cec9cb58dddcb85a7dd70e289356d55564902949da2a21844b38ef93e
 * This is a preflight/patch primitive only, NOT an installed game mod.
 * All slot updates must occur after ffd_resolve_imports and before so_finalize.
 * This loader relocates the writable image at load_base, not load_virtbase.
 */
#ifndef FFD_SWITCH_PROFILE_H
#define FFD_SWITCH_PROFILE_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
    const char *symbol;
    uintptr_t entry_offset;
    uintptr_t got_offset;
} FfdSwitchSite;

enum {
    FFD_SWITCH_KEY_STATE,
    FFD_SWITCH_KEY_TRIGGER,
    FFD_SWITCH_KEY_REPEAT,
    FFD_SWITCH_MENU_UPDATE,
    FFD_SWITCH_MENU_DRAW,
    FFD_SWITCH_MAIN_VIEWPORT,
    FFD_SWITCH_GFX_VIEWPORT,
    FFD_SWITCH_GFX_2D_MATRIX,
    FFD_SWITCH_FIELD_DRAW,
    FFD_SWITCH_FIELD_FAR,
    FFD_SWITCH_FIELD_MINIMAP,
    FFD_SWITCH_FIELD_MESSAGE,
    FFD_SWITCH_GFX_DEST_WIDTH,
    FFD_SWITCH_GFX_DEST_HEIGHT,
    FFD_SWITCH_GET_TOUCH_INFO,
    FFD_SWITCH_TOUCH_SET_DATA,
    FFD_SWITCH_GFX_DRAW_IMAGE_EX,
    FFD_SWITCH_GFX_FILL4,
    FFD_SWITCH_GFX_FILL8,
    FFD_SWITCH_SITE_COUNT
};

static const FfdSwitchSite ffd_switch_sites[FFD_SWITCH_SITE_COUNT] = {
    {"_ZN9GameClass8KeyStateEi", 0x3ec7bcUL, 0x45c2a8UL},
    {"_ZN9GameClass10KeyTriggerEi", 0x3dd08cUL, 0x45b208UL},
    {"_ZN9GameClass9KeyRepeatEi", 0x3ec7d4UL, 0x45b2f0UL},
    {"_ZN10MenuWidget6UpdateEv", 0x413478UL, 0x45b288UL},
    {"_ZN10MenuWidget4DrawER13MtxGraphics3D", 0x413878UL, 0x45b388UL},
    {"_ZN9MainScene11SetViewPortER13MtxGraphics3D", 0x3f6654UL, 0x45d2e0UL},
    {"_ZN13MtxGraphics3D11SetViewPortEiiii", 0x370f28UL, 0x45a840UL},
    {"_ZN13MtxGraphics3D15Set2DModeMatrixEff", 0x371b84UL, 0x45a8e0UL},
    {"_ZN10FieldClass12DrawMapFieldEP13MtxGraphics3Df", 0x3beb08UL, 0x45c2e0UL},
    {"_ZN10FieldClass7DrawFarEP13MtxGraphics3D", 0x3c97c8UL, 0x45c728UL},
    {"_ZN10FieldClass11DrawMiniMapEP13MtxGraphics3D", 0x3caf24UL, 0x45c2f0UL},
    {"_ZN10FieldClass11DrawMessageEP13MtxGraphics3D", 0x3c5170UL, 0x45c318UL},
    {"_ZN13MtxGraphics3D19GetDestinationWidthEv", 0x370d7cUL, 0x45a818UL},
    {"_ZN13MtxGraphics3D20GetDestinationHeightEv", 0x370da8UL, 0x45a820UL},
    {"_Z12GetTouchInfov", 0x37ff10UL, 0x45afc0UL},
    {"_ZN13MtxTouchPanel7SetDataEiffi", 0x37f7a8UL, 0x45af40UL},
    {"_ZN13MtxGraphics3D11DrawImageExER10MtxTextureffffffffiii", 0x373a90UL, 0x45a320UL},
    {"_ZN13MtxGraphics3D8FillRectEiiii", 0x372c38UL, 0x45a510UL},
    {"_ZN13MtxGraphics3D8FillRectEiiiiiiii", 0x372d30UL, 0x45a970UL},
};

/* Verifies both the ELF's named symbol and the *already resolved* GOT.
 * Fails closed with no writes if the expected game profile is absent.
 * This cannot substitute for a cryptographic hash of the complete ELF. */
static int ffd_switch_validate_site(so_module *mod, unsigned index)
{
    if (!mod || index >= FFD_SWITCH_SITE_COUNT || !mod->load_base || !mod->load_virtbase ||
        !mod->syms || !mod->dynstrtab) return 0;
    const FfdSwitchSite *s = &ffd_switch_sites[index];
    if (s->got_offset > mod->load_size ||
        mod->load_size - s->got_offset < sizeof(uintptr_t) ||
        s->entry_offset >= mod->load_size) return 0;
    uintptr_t base = (uintptr_t)mod->load_virtbase;
    uintptr_t actual = so_try_find_addr_rx(mod, s->symbol);
    if (actual != base + s->entry_offset) return 0;
    /* Relocations are applied to load_base before so_finalize maps it at
     * load_virtbase. Never dereference unmapped load_virtbase here. */
    return *(const volatile uintptr_t *)((uintptr_t)mod->load_base + s->got_offset) == actual;
}

typedef struct {
    unsigned site_index;
    uintptr_t replacement;
    uintptr_t *original; /* assigned only when every hook passes preflight */
} FfdSwitchPatch;

/* The caller must first ensure this module is the expected executable via
 * exact SHA256 (offline), then check that original slot values still match.
 * The installer makes no partial writes if any slot is stale/invalid.
 * It is intended only for code called from a single-threaded boot path. */
static int ffd_switch_install_patches(so_module *mod, const FfdSwitchPatch *patches, size_t count)
{
    if (!mod || !patches || !count || count > FFD_SWITCH_SITE_COUNT) return 0;
    for (size_t i = 0; i < count; ++i) {
        if (patches[i].site_index >= FFD_SWITCH_SITE_COUNT ||
            !patches[i].replacement || !patches[i].original ||
            !ffd_switch_validate_site(mod, patches[i].site_index)) return 0;
        for (size_t j = 0; j < i; ++j)
            if (patches[j].site_index == patches[i].site_index) return 0;
    }
    uintptr_t base = (uintptr_t)mod->load_virtbase;
    for (size_t i = 0; i < count; ++i) {
        const FfdSwitchSite *s = &ffd_switch_sites[patches[i].site_index];
        *patches[i].original = base + s->entry_offset;
        *(volatile uintptr_t *)((uintptr_t)mod->load_base + s->got_offset) = patches[i].replacement;
    }
    return 1;
}

#endif
