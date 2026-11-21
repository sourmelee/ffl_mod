/* Vita 1.1.3 experimental navigation adapter.
 * Include only in the original ffd-vita/loader/main.c after main_mod is declared.
 * Uses 32-bit ARM Thumb ELF export/GOT validation and the shared menu algorithm.
 * Device runtime NOT verified. Enabled only by enhanced cfg.
 */
#ifndef FFD_VITA_NAV_ADAPTER_H
#define FFD_VITA_NAV_ADAPTER_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ffd_vita_profile.h"
#include "ffd_nav.h"
#include "ffd_display_transform.h"

#define FFD_VITA_ITEM_ACTIVE 0x08
#define FFD_VITA_ITEM_VISIBLE 0x09
#define FFD_VITA_ITEM_TOUCHABLE 0x10
#define FFD_VITA_ITEM_POS 0x1c
#define FFD_VITA_HEAD 0x10
#define FFD_VITA_NODE_ITEM 0x08

static uintptr_t fv_old_widget_draw;
static FfdNavCandidate fv_candidates[FFD_NAV_MAX_CANDIDATES];
static int fv_candidates_count, fv_enabled, fv_cursor_valid, fv_pressed, fv_tap_pending;
static FfdNavState fv_cursor = {0};
static float fv_cursor_x, fv_cursor_y;
static uint32_t fv_prev_buttons;

static int fv_config_has(const char *key)
{
    FILE *f = fopen("ux0:data/ffd/ffdvita_enhanced.cfg", "r");
    if (!f) return 0;
    char buf[128];
    int result = 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (!strncmp(buf, key, strlen(key)) && buf[strlen(key)] == '1')
            result = 1;
    }
    fclose(f);
    return result;
}
static float fv_load_float(const void *p, unsigned off)
{
    float v;
    memcpy(&v, (const char *)p + off, sizeof(v));
    return v;
}
static void fv_draw_widget(void *widget, void *gfx)
{
    ((void (*)(void *, void *))fv_old_widget_draw)(widget, gfx);
    if (!fv_enabled || !widget || fv_candidates_count >= FFD_NAV_MAX_CANDIDATES)
        return;
    uintptr_t node = *(uintptr_t *)((uintptr_t)widget + FFD_VITA_HEAD);
    for (int i = 0; i < 64 && node && fv_candidates_count < FFD_NAV_MAX_CANDIDATES; ++i) {
        uintptr_t item = *(uintptr_t *)(node + FFD_VITA_NODE_ITEM);
        uintptr_t next = *(uintptr_t *)node;
        if (item) {
            const unsigned char *flags = (const unsigned char *)item;
            if (flags[FFD_VITA_ITEM_ACTIVE] && flags[FFD_VITA_ITEM_VISIBLE] &&
                flags[FFD_VITA_ITEM_TOUCHABLE]) {
                float x = fv_load_float(flags, FFD_VITA_ITEM_POS);
                float y = fv_load_float(flags, FFD_VITA_ITEM_POS + 4);
                float w = fv_load_float(flags, FFD_VITA_ITEM_POS + 8);
                float h = fv_load_float(flags, FFD_VITA_ITEM_POS + 12);
                if (w > 3 && w < 640 && h > 3 && h < 384 &&
                    x > -128 && x < 768 && y > -128 && y < 512) {
                    int seen = 0;
                    for (int j = 0; j < fv_candidates_count; ++j)
                        if (fv_candidates[j].item == item) { seen = 1; break; }
                    if (!seen) {
                        FfdNavCandidate *c = &fv_candidates[fv_candidates_count++];
                        memset(c, 0, sizeof(*c));
                        c->item = item;
                        c->widget = (uintptr_t)widget;
                        c->sub = -1;
                        c->x = x; c->y = y; c->w = w; c->h = h;
                    }
                }
            }
        }
        if (next == node) break;
        node = next;
    }
}
static int fv_install_navigation(so_module *mod)
{
    if (!fv_config_has("game_aware_navigation=")) return 0;
    const unsigned required[] = {FFD_VITA_MENU_DRAW};
    if (!ffd_vita_validate_group(mod, required, 1)) return 0;
    fv_old_widget_draw = so_symbol(mod, ffd_vita_sites[FFD_VITA_MENU_DRAW].symbol);
    if (!fv_old_widget_draw) return 0;
    ffd_vita_replace_slot(mod, FFD_VITA_MENU_DRAW, (uintptr_t)fv_draw_widget);
    fv_enabled = 1;
    return 1;
}
/* Called BEFORE the original controller-to-touch logic on each iteration.
 * L+directions navigate actual, last-rendered widget candidates.
 * Returning 1 suppresses legacy D-pad/Cross virtual touches for this frame.
 */
static int fv_pad_frame(uint32_t buttons)
{
    int mode = fv_enabled && (buttons & SCE_CTRL_LTRIGGER) && fv_candidates_count;
    fv_pressed = 0;
    fv_cursor_valid = 0;
    if (!mode) { fv_prev_buttons = buttons; return 0; }
    uint32_t edge = buttons & ~fv_prev_buttons;
    unsigned direction = 0;
    if (edge & SCE_CTRL_LEFT) direction = FFD_NAV_LEFT;
    else if (edge & SCE_CTRL_RIGHT) direction = FFD_NAV_RIGHT;
    else if (edge & SCE_CTRL_UP) direction = FFD_NAV_UP;
    else if (edge & SCE_CTRL_DOWN) direction = FFD_NAV_DOWN;
    if (edge & SCE_CTRL_CROSS) direction |= FFD_NAV_ACCEPT;
    FfdNavResult r = ffd_nav_step(&fv_cursor, fv_candidates,
                                 fv_candidates_count, 0, direction);
    if (r.selected_index >= 0) {
        FfdPoint p = ffd_game_to_surface(r.touch_x, r.touch_y,
                                         (float)SCREEN_W, (float)SCREEN_H);
        fv_cursor_x = p.x;
        fv_cursor_y = p.y;
        fv_cursor_valid = 1;
        fv_pressed = r.activated;
    }
    fv_prev_buttons = buttons;
    return fv_cursor_valid;
}
typedef void (*FvTouch)(void *, void *, int, int, float, float);
static void fv_send_touch(FvTouch fn, void *env, int occupied_slots)
{
    if (!fv_enabled || !fn || occupied_slots >= 2) {
        fv_tap_pending = 0;
        return;
    }
    if (fv_tap_pending) {
        fn(env, 0, 2, occupied_slots, fv_cursor_x, fv_cursor_y);
        fv_tap_pending = 0;
    }
    if (fv_cursor_valid && fv_pressed) {
        fn(env, 0, 0, occupied_slots, fv_cursor_x, fv_cursor_y);
        fv_tap_pending = 1;
    }
}
static void fv_before_render(void) { fv_candidates_count = 0; }
#endif
