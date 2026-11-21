/* Vita 1.1.3 experimental projection adapter.
 * Native viewport / matrix hooks only: expanded-field camera, tiles, HUD,
 * backgrounds, input and scissor alignment are NOT fully reconstructed.
 * Must be explicitly enabled; not hardware tested.
 */
#ifndef FFD_VITA_WIDE_ADAPTER_H
#define FFD_VITA_WIDE_ADAPTER_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ffd_vita_profile.h"
#include "ffd_display_transform.h"
#define FV_GFX_VP 0x4ac
typedef void (*FvViewport)(void *, int, int, int, int);
typedef void (*FvMainViewport)(void *, void *);
typedef void (*FvMatrix2D)(void *, float, float);
typedef void (*FvMode)(void *, int);
typedef void (*FvIdent)(void *);
typedef void (*FvOrtho)(void *, float, float, float, float, float, float);
static FvViewport fv_orig_viewport;
static FvMainViewport fv_orig_mainviewport;
static FvMatrix2D fv_orig_matrix;
static FvMode fv_mode;
static FvIdent fv_ident;
static FvOrtho fv_ortho;
static int fv_wide_enabled, fv_main_call, fv_view_ready;
static float fv_extra_width;
static void fv_viewport_hook(void *gfx, int x, int y, int w, int h)
{
    if (fv_main_call && x > 0 && y == 0 && h == SCREEN_H &&
        w + 2*x >= SCREEN_W - 2 && w + 2*x <= SCREEN_W + 2)
        fv_orig_viewport(gfx, 0, 0, SCREEN_W, SCREEN_H);
    else
        fv_orig_viewport(gfx, x, y, w, h);
}
static void fv_mainviewport_hook(void *scene, void *gfx)
{
    fv_main_call = 1;
    fv_orig_mainviewport(scene, gfx);
    fv_main_call = 0;
    float w, h;
    memcpy(&w, (char *)gfx + FV_GFX_VP + 8, sizeof(w));
    memcpy(&h, (char *)gfx + FV_GFX_VP + 12, sizeof(h));
    fv_view_ready = (w == (float)SCREEN_W && h == (float)SCREEN_H);
}
static void fv_matrix_hook(void *gfx, float w, float h)
{
    if (!fv_view_ready || w != 640 || h != 384 || fv_extra_width <= 0) {
        fv_orig_matrix(gfx, w, h);
        return;
    }
    fv_mode(gfx, 1);
    fv_ident(gfx);
    fv_ortho(gfx, -fv_extra_width, 640.0f + fv_extra_width,
             384.0f, 0.0f, 0.0f, 1024.0f);
    fv_mode(gfx, 0);
    fv_ident(gfx);
}
static int fv_install_widescreen(so_module *mod)
{
    if (!fv_config_has("experimental_widescreen=")) return 0;
    const unsigned sites[] = {FFD_VITA_GFX_VIEWPORT,
                              FFD_VITA_MAIN_VIEWPORT, FFD_VITA_GFX_2D_MATRIX};
    if (!ffd_vita_validate_group(mod, sites, 3)) return 0;
    fv_mode = (FvMode)so_symbol(mod, "_ZN13MtxGraphics3D13SetMatrixModeEi");
    fv_ident = (FvIdent)so_symbol(mod, "_ZN13MtxGraphics3D12LoadIdentiryEv");
    fv_ortho = (FvOrtho)so_symbol(mod, "_ZN13MtxGraphics3D5OrthoEffffff");
    if (!fv_mode || !fv_ident || !fv_ortho) return 0;
    fv_orig_viewport = (FvViewport)so_symbol(mod, ffd_vita_sites[FFD_VITA_GFX_VIEWPORT].symbol);
    fv_orig_mainviewport = (FvMainViewport)so_symbol(mod, ffd_vita_sites[FFD_VITA_MAIN_VIEWPORT].symbol);
    fv_orig_matrix = (FvMatrix2D)so_symbol(mod, ffd_vita_sites[FFD_VITA_GFX_2D_MATRIX].symbol);
    if (!fv_orig_viewport || !fv_orig_mainviewport || !fv_orig_matrix) return 0;
    DisplayGeometry g = display_geometry((float)SCREEN_W, (float)SCREEN_H);
    fv_extra_width = g.y == 0 ? g.x : 0;
    if (fv_extra_width <= 0) return 0;
    ffd_vita_replace_slot(mod, FFD_VITA_GFX_VIEWPORT, (uintptr_t)fv_viewport_hook);
    ffd_vita_replace_slot(mod, FFD_VITA_MAIN_VIEWPORT, (uintptr_t)fv_mainviewport_hook);
    ffd_vita_replace_slot(mod, FFD_VITA_GFX_2D_MATRIX, (uintptr_t)fv_matrix_hook);
    fv_wide_enabled = 1;
    return 1;
}
#endif
