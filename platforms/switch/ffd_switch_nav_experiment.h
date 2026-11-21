/* Experimental Switch integration. Include in source/main.c AFTER so_util.h and
 * use the accompanying apply_switch_loader.py. 1.1.9 engine profile only.
 *
 * ZL + D-pad / left stick navigates a discovered ordinary MenuItem; ZL + A
 * sends a game touch to the item's centre. This deliberately avoids stealing
 * plain D-pad movement in the field. Experimental and OFF by default.
 */
#ifndef FFD_SWITCH_NAV_EXPERIMENT_H
#define FFD_SWITCH_NAV_EXPERIMENT_H
#include "../../portable/ffd_nav.h"
#include "../../portable/ffd_display_transform.h"
#include "ffd_switch_profile.h"
#include "ffd_switch_menu_layout.h"

static uintptr_t ffd_nx_original_menu_draw = 0;
static FfdNavCandidate ffd_nx_visible[FFD_NAV_MAX_CANDIDATES];
static int ffd_nx_visible_count = 0;
static FfdNavState ffd_nx_selected = {0};
static FfdSwitchNavRepeat ffd_nx_repeat = {0};
static int ffd_nx_enabled = 0;
static int ffd_nx_nav_active = 0;
static float ffd_nx_nav_x = 0.0f, ffd_nx_nav_y = 0.0f;

static int ffd_nx_read_enabled(void)
{
    /* Sidecar is opt-in and independent of the upstream config format. */
    FILE *f = fopen("enhanced.txt", "r");
    if (!f) return 0;
    char line[128];
    int enabled = 0;
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "game_aware_navigation=%d", &enabled) == 1) break;
    fclose(f);
    return enabled == 1;
}

static void ffd_nx_menu_draw_hook(void *widget, void *graphics)
{
    /* Read candidate layout while the original engine widget is valid. */
    if (ffd_nx_enabled && ffd_nx_visible_count < FFD_NAV_MAX_CANDIDATES) {
        int remaining = FFD_NAV_MAX_CANDIDATES - ffd_nx_visible_count;
        ffd_nx_visible_count += ffd_switch_collect_menu_items(
            widget, ffd_nx_visible + ffd_nx_visible_count, remaining);
    }
    ((void (*)(void *, void *))ffd_nx_original_menu_draw)(widget, graphics);
}

static int ffd_nx_install_navigation(so_module *mod)
{
    if (!ffd_nx_read_enabled()) return 0;
    /* Validate both widget exports, not only the patched one. */
    if (!ffd_switch_validate_site(mod, FFD_SWITCH_MENU_UPDATE) ||
        !ffd_switch_validate_site(mod, FFD_SWITCH_MENU_DRAW)) return 0;
    const FfdSwitchPatch hooks[] = {
        {FFD_SWITCH_MENU_DRAW, (uintptr_t)ffd_nx_menu_draw_hook,
         &ffd_nx_original_menu_draw}
    };
    if (!ffd_switch_install_patches(mod, hooks, 1)) return 0;
    ffd_nx_enabled = 1;
    return 1;
}

static void ffd_nx_begin_render(void)
{
    /* Invoked immediately before e_render(). Cache is then ready for the
     * following update_input() call. */
    ffd_nx_visible_count = 0;
}

/* Returns 1 when the normal D-pad/A-on-screen simulated touches should be
 * suppressed. The real touchscreen and BACK button keep their original path.
 * Caller supplies hardware flags, so this can also be host tested. */
static int ffd_nx_navigation_input(int zl, int left, int right,
                                   int up, int down, int accept,
                                   int screen_width, int screen_height)
{
    ffd_nx_nav_active = 0;
    unsigned held = (left ? FFD_NAV_LEFT : 0u) |
                    (right ? FFD_NAV_RIGHT : 0u) |
                    (up ? FFD_NAV_UP : 0u) |
                    (down ? FFD_NAV_DOWN : 0u) |
                    (accept ? FFD_NAV_ACCEPT : 0u);
    if (!ffd_nx_enabled || !zl || !ffd_nx_visible_count) {
        ffd_switch_nav_triggers(&ffd_nx_repeat, 0);
        return 0;
    }
    unsigned trigger = ffd_switch_nav_triggers(&ffd_nx_repeat, held);
    FfdNavResult result = ffd_nav_step(&ffd_nx_selected, ffd_nx_visible,
                                      ffd_nx_visible_count, 0, trigger);
    if (result.selected_index < 0) return 0;
    FfdPoint physical = ffd_game_to_surface(result.touch_x, result.touch_y,
                                           (float)screen_width,
                                           (float)screen_height);
    ffd_nx_nav_x = physical.x;
    ffd_nx_nav_y = physical.y;
    ffd_nx_nav_active = 1;
    return 1;
}

#endif
