/* Switch-only, 16:9 expanded-field rendering experiment.
 * Derived from the FFLmod Android widescreen method, with the original
 * uploaded ARM64 libjniproxy.so profile and a completely separate toggle.
 *
 * EXPERIMENTAL: no Switch runtime verification. Some widgets, effects,
 * camera-clamp edge cases and fades may require further adjustment.
 */
#ifndef FFD_SWITCH_WIDE_EXPERIMENT_H
#define FFD_SWITCH_WIDE_EXPERIMENT_H
#include <stdio.h>
#include <stdint.h>
#include "../../portable/ffd_display_transform.h"
#include "ffd_switch_profile.h"

#define NX_VP        0x4c0u
#define NX_PROJ      0x5cu
#define NX_MODELVIEW 0x1cu
#define NX_RATIO     0x5b4u
#define NX_TP_SCALE  0x674u
#define NX_TP_OFFSET 0x67cu
#define NX_FIELD_W   0x0cu
#define NX_FIELD_H   0x10u
#define NX_APP_SLOT  0x4599d8u
#define NX_APP_GLOB  0x45f1e8u

#define NX_F32(obj,offset) (*(volatile float *)((uintptr_t)(obj)+(offset)))
#define NX_PTR(obj,offset) (*(volatile uintptr_t *)((uintptr_t)(obj)+(offset)))

typedef void (*NxViewport)(void *, int, int, int, int);
typedef void (*NxSceneViewport)(void *, void *);
typedef void (*NxMatrix2D)(void *, float, float);
typedef void (*NxImageEx)(void *, void *, float, float, float, float,
                          float, float, float, float, int, int, int);
typedef void (*NxFill4)(void *, int, int, int, int);
typedef void (*NxFill8)(void *, int, int, int, int, int, int, int, int);
typedef void (*NxDrawField)(void *, void *);
typedef void (*NxMode)(void *, int);
typedef void (*NxIdent)(void *);
typedef void (*NxOrtho)(void *, float, float, float, float, float, float);
typedef int (*NxDestination)(void *);
typedef void *(*NxTouchInfo)(void *);
typedef void *(*NxGetTouch)(void);
typedef void (*NxFieldMode)(void *, void *);

static uintptr_t nx_wide_base;
static int nx_wide_installed, nx_in_main_view;
static int nx_dest_w, nx_dest_h;
static float nx_extra_x, nx_scale, nx_proj0, nx_proj5, nx_proj12, nx_proj13;
static uintptr_t nx_orig_viewport, nx_orig_main_viewport, nx_orig_matrix2d;
static uintptr_t nx_orig_image, nx_orig_fill4, nx_orig_fill8;
static uintptr_t nx_orig_draw_far, nx_orig_minimap;
static uintptr_t nx_set_mode, nx_load_ident, nx_ortho, nx_get_width, nx_get_height;
static uintptr_t nx_get_touch, nx_field_mode;

static int nx_wide_requested(void)
{
    FILE *f = fopen("enhanced.txt", "r");
    if (!f) return 0;
    char line[128];
    int enabled = 0;
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "experimental_widescreen=%d", &enabled) == 1) break;
    fclose(f);
    return enabled == 1;
}

static void nx_wide_projection(void *gfx, float shift)
{
    /* The 1.1.9 symbols were resolved by name before so_finalize. */
    ((NxMode)nx_set_mode)(gfx, 1);
    ((NxIdent)nx_load_ident)(gfx);
    ((NxOrtho)nx_ortho)(gfx, -nx_extra_x - shift,
                        640.0f + nx_extra_x - shift,
                        384.0f, 0.0f, 0.0f, 1024.0f);
    ((NxMode)nx_set_mode)(gfx, 0);
    NX_F32(gfx, NX_VP) = (nx_extra_x + shift) * nx_scale;
}

static int nx_wide_is_2d(void *gfx)
{
    return nx_wide_installed && nx_extra_x > 0.0f && gfx &&
        NX_F32(gfx, NX_PROJ) == nx_proj0 &&
        NX_F32(gfx, NX_PROJ+20) == nx_proj5 &&
        NX_F32(gfx, NX_PROJ+48) == nx_proj12 &&
        NX_F32(gfx, NX_PROJ+52) == nx_proj13;
}

static int nx_wide_is_flat(void *gfx)
{
    if (!nx_wide_is_2d(gfx)) return 0;
    const volatile float *m = (const volatile float *)((uintptr_t)gfx+NX_MODELVIEW);
    return m[0] == 1.0f && m[5] == 1.0f &&
           m[1] == 0.0f && m[4] == 0.0f &&
           m[12] == 0.0f && m[13] == 0.0f;
}

static void nx_wide_viewport_hook(void *gfx, int x, int y, int w, int h)
{
    if (nx_in_main_view && nx_dest_w > 0 && nx_dest_h > 0 &&
        x >= 0 && y >= 0 && w > 0 && h > 0) {
        ((NxViewport)nx_orig_viewport)(gfx, 0, 0, nx_dest_w, nx_dest_h);
        return;
    }
    /* Other game viewport operations remain untouched. */
    ((NxViewport)nx_orig_viewport)(gfx, x, y, w, h);
}

static void nx_wide_main_viewport_hook(void *scene, void *gfx)
{
    nx_dest_w = ((NxDestination)nx_get_width)(gfx);
    nx_dest_h = ((NxDestination)nx_get_height)(gfx);
    nx_in_main_view = 1;
    ((NxSceneViewport)nx_orig_main_viewport)(scene, gfx);
    nx_in_main_view = 0;

    if (nx_dest_w <= 0 || nx_dest_h <= 0 ||
        NX_F32(gfx, NX_VP+8) != (float)nx_dest_w ||
        NX_F32(gfx, NX_VP+12) != (float)nx_dest_h) {
        nx_extra_x = 0.0f;
        return;
    }

    DisplayGeometry geometry = display_geometry((float)nx_dest_w,
                                                  (float)nx_dest_h);
    /* The experiment only targets the original wide 16:9 canvas. */
    if (!geometry.active || geometry.y > 0.0f || geometry.x <= 0.0f) {
        nx_extra_x = 0.0f;
        return;
    }
    nx_extra_x = geometry.x;
    nx_scale = geometry.scale;

    /* Match FFLmod's 16:9 touch/scissor coordinates after the game resets them. */
    void *tp = ((NxGetTouch)nx_get_touch)();
    if (tp) {
        NX_F32(tp, NX_TP_SCALE) = 1.0f / nx_scale;
        NX_F32(tp, NX_TP_SCALE+4) = 1.0f / nx_scale;
        NX_F32(tp, NX_TP_OFFSET) = nx_extra_x * nx_scale;
        NX_F32(tp, NX_TP_OFFSET+4) = 0.0f;
    }
    if (NX_PTR((void *)nx_wide_base, NX_APP_SLOT) ==
        nx_wide_base + NX_APP_GLOB) {
        uintptr_t app = NX_PTR((void *)nx_wide_base, NX_APP_GLOB);
        if (app) {
            NX_F32((void *)app, 0x188u) = 1.0f;
            NX_F32((void *)app, 0x18cu) = 1.0f;
        }
    }
    NX_F32(gfx, NX_VP) = nx_extra_x * nx_scale;
    NX_F32(gfx, NX_RATIO) = 1.0f / nx_scale;
    NX_F32(gfx, NX_RATIO+4) = 1.0f / nx_scale;
}

static void nx_wide_matrix2d_hook(void *gfx, float w, float h)
{
    if (nx_extra_x <= 0.0f || w != 640.0f || h != 384.0f) {
        ((NxMatrix2D)nx_orig_matrix2d)(gfx, w, h);
        return;
    }
    nx_wide_projection(gfx, 0.0f);
    ((NxIdent)nx_load_ident)(gfx);
    nx_proj0 = NX_F32(gfx, NX_PROJ);
    nx_proj5 = NX_F32(gfx, NX_PROJ+20);
    nx_proj12 = NX_F32(gfx, NX_PROJ+48);
    nx_proj13 = NX_F32(gfx, NX_PROJ+52);
}

static void nx_wide_image_hook(void *gfx, void *tex,
                               float x, float y, float w, float h,
                               float u, float v, float uw, float vh,
                               int flags, int color, int mode)
{
    if (!(flags & 7) && nx_wide_is_flat(gfx)) {
        DisplayRect rect = {x, y, w, h};
        rect = display_expand_image(rect, nx_extra_x, 0.0f);
        x = rect.x; y = rect.y; w = rect.w; h = rect.h;
    }
    ((NxImageEx)nx_orig_image)(gfx, tex, x, y, w, h,
                              u, v, uw, vh, flags, color, mode);
}

static void nx_wide_fill4_hook(void *gfx, int x, int y, int w, int h)
{
    if (nx_wide_is_flat(gfx))
        display_expand_fill(&x, &y, &w, &h, nx_extra_x, 0.0f);
    ((NxFill4)nx_orig_fill4)(gfx, x, y, w, h);
}

static void nx_wide_fill8_hook(void *gfx, int x, int y, int w, int h,
                               int red, int green, int blue, int alpha)
{
    if (nx_wide_is_flat(gfx))
        display_expand_fill(&x, &y, &w, &h, nx_extra_x, 0.0f);
    ((NxFill8)nx_orig_fill8)(gfx, x, y, w, h, red, green, blue, alpha);
}

static void nx_wide_draw_far_hook(void *field, void *gfx)
{
    if (nx_extra_x <= 0.0f) {
        ((NxDrawField)nx_orig_draw_far)(field, gfx);
        return;
    }
    float field_w = NX_F32(field, NX_FIELD_W);
    float field_h = NX_F32(field, NX_FIELD_H);
    if (field_w <= 640.5f && field_h <= 384.5f) {
        ((NxDrawField)nx_orig_draw_far)(field, gfx);
        return;
    }
    NX_F32(field, NX_FIELD_W) = 640.0f;
    NX_F32(field, NX_FIELD_H) = 384.0f;
    ((NxFieldMode)nx_field_mode)(field, gfx);
    ((NxDrawField)nx_orig_draw_far)(field, gfx);
    NX_F32(field, NX_FIELD_W) = field_w;
    NX_F32(field, NX_FIELD_H) = field_h;
    ((NxFieldMode)nx_field_mode)(field, gfx);
}

static void nx_wide_minimap_hook(void *field, void *gfx)
{
    int shift = nx_wide_is_2d(gfx);
    if (shift) nx_wide_projection(gfx, nx_extra_x);
    ((NxDrawField)nx_orig_minimap)(field, gfx);
    if (shift) nx_wide_projection(gfx, 0.0f);
}

static int ffd_nx_install_widescreen(so_module *mod)
{
    if (!nx_wide_requested() || !mod) return 0;

    const unsigned required[] = {
        FFD_SWITCH_GFX_VIEWPORT, FFD_SWITCH_MAIN_VIEWPORT,
        FFD_SWITCH_GFX_2D_MATRIX, FFD_SWITCH_GFX_DRAW_IMAGE_EX,
        FFD_SWITCH_GFX_FILL4, FFD_SWITCH_GFX_FILL8,
        FFD_SWITCH_FIELD_FAR, FFD_SWITCH_FIELD_MINIMAP,
        FFD_SWITCH_GET_TOUCH_INFO, FFD_SWITCH_GFX_DEST_WIDTH,
        FFD_SWITCH_GFX_DEST_HEIGHT
    };
    for (unsigned i = 0; i < sizeof(required)/sizeof(required[0]); ++i)
        if (!ffd_switch_validate_site(mod, required[i])) return 0;
    static const struct { const char *name; uintptr_t *dst; } support[] = {
        {"_ZN13MtxGraphics3D13SetMatrixModeEi", &nx_set_mode},
        {"_ZN13MtxGraphics3D12LoadIdentiryEv", &nx_load_ident},
        {"_ZN13MtxGraphics3D5OrthoEffffff", &nx_ortho},
        {"_ZN13MtxGraphics3D19GetDestinationWidthEv", &nx_get_width},
        {"_ZN13MtxGraphics3D20GetDestinationHeightEv", &nx_get_height},
        {"_Z12GetTouchInfov", &nx_get_touch},
        {"_ZN10FieldClass9Set2DModeEP13MtxGraphics3D", &nx_field_mode}
    };
    for (unsigned i = 0; i < sizeof(support)/sizeof(support[0]); ++i) {
        uintptr_t a = so_try_find_addr_rx(mod, support[i].name);
        if (!a) return 0;
        *support[i].dst = a;
    }

    const FfdSwitchPatch group[] = {
        {FFD_SWITCH_GFX_VIEWPORT, (uintptr_t)nx_wide_viewport_hook,
         &nx_orig_viewport},
        {FFD_SWITCH_MAIN_VIEWPORT, (uintptr_t)nx_wide_main_viewport_hook,
         &nx_orig_main_viewport},
        {FFD_SWITCH_GFX_2D_MATRIX, (uintptr_t)nx_wide_matrix2d_hook,
         &nx_orig_matrix2d},
        {FFD_SWITCH_GFX_DRAW_IMAGE_EX, (uintptr_t)nx_wide_image_hook,
         &nx_orig_image},
        {FFD_SWITCH_GFX_FILL4, (uintptr_t)nx_wide_fill4_hook,
         &nx_orig_fill4},
        {FFD_SWITCH_GFX_FILL8, (uintptr_t)nx_wide_fill8_hook,
         &nx_orig_fill8},
        {FFD_SWITCH_FIELD_FAR, (uintptr_t)nx_wide_draw_far_hook,
         &nx_orig_draw_far},
        {FFD_SWITCH_FIELD_MINIMAP, (uintptr_t)nx_wide_minimap_hook,
         &nx_orig_minimap}
    };
    nx_wide_base = (uintptr_t)mod->load_virtbase;
    if (!ffd_switch_install_patches(mod, group,
                                    sizeof(group)/sizeof(group[0]))) return 0;
    nx_wide_installed = 1;
    return 1;
}

#endif
