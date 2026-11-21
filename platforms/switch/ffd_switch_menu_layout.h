/* Switch 1.1.9 menu layout adapter, verified against the uploaded AArch64
 * MenuWidget::Update/Draw disassembly. Plain C; no Switch SDK required.
 * Not a standalone hook: invoke only from a validated MenuWidget hook.
 */
#ifndef FFD_SWITCH_MENU_LAYOUT_H
#define FFD_SWITCH_MENU_LAYOUT_H
#include <stdint.h>
#include <stddef.h>
#include "../../portable/ffd_nav.h"
#define FFD_SWITCH_WIDGET_HEAD 0x20u
#define FFD_SWITCH_NODE_NEXT   0x00u
#define FFD_SWITCH_NODE_ITEM   0x10u
#define FFD_SWITCH_ITEM_ACTIVE 0x0cu
#define FFD_SWITCH_ITEM_VISIBLE 0x0du
#define FFD_SWITCH_ITEM_TOUCHABLE 0x14u
#define FFD_SWITCH_ITEM_X 0x28u
#define FFD_SWITCH_ITEM_Y 0x2cu
#define FFD_SWITCH_ITEM_W 0x30u
#define FFD_SWITCH_ITEM_H 0x34u

static int ffd_switch_collect_menu_items(const void *widget,
                                         FfdNavCandidate *out, int capacity)
{
    if (!widget || !out || capacity <= 0) return 0;
    if (capacity > FFD_NAV_MAX_CANDIDATES) capacity = FFD_NAV_MAX_CANDIDATES;
    uintptr_t node = *(const uintptr_t *)((uintptr_t)widget + FFD_SWITCH_WIDGET_HEAD);
    int count = 0;
    for (int guard = 0; node && guard < 160; guard++) {
        uintptr_t item = *(const uintptr_t *)(node + FFD_SWITCH_NODE_ITEM);
        if (!item) break; /* corrupt/incomplete list: fail conservatively */
        const unsigned char *p = (const unsigned char *)item;
        if (p[FFD_SWITCH_ITEM_ACTIVE] && p[FFD_SWITCH_ITEM_VISIBLE] &&
            p[FFD_SWITCH_ITEM_TOUCHABLE]) {
            FfdNavCandidate c = {0};
            c.item = item;
            c.widget = (uintptr_t)widget;
            c.sub = -1;
            c.list = 0; /* menu subitem grids need separate per-type adapters */
            c.x = *(const float *)(item + FFD_SWITCH_ITEM_X);
            c.y = *(const float *)(item + FFD_SWITCH_ITEM_Y);
            c.w = *(const float *)(item + FFD_SWITCH_ITEM_W);
            c.h = *(const float *)(item + FFD_SWITCH_ITEM_H);
            /* Exclude nonsensical geometry and overflow/NaN values. */
            if (c.w > 0.0f && c.h > 0.0f &&
                c.w <= 4096.0f && c.h <= 4096.0f &&
                c.x > -4096.0f && c.x < 4096.0f &&
                c.y > -4096.0f && c.y < 4096.0f &&
                count < capacity) out[count++] = c;
        }
        uintptr_t next = *(const uintptr_t *)(node + FFD_SWITCH_NODE_NEXT);
        if (next == node) break;
        node = next;
    }
    return count;
}

typedef struct {
    unsigned held_before;
    unsigned hold_frames[6];
} FfdSwitchNavRepeat;

/* Game runs at ~30fps. FFLmod's initial 12-frame, then 3-frame repeat.
 * Output edge/auto-repeat triggers for portable navigation. */
static unsigned ffd_switch_nav_triggers(FfdSwitchNavRepeat *state, unsigned held)
{
    if (!state) return 0;
    const unsigned bits[6] = {FFD_NAV_LEFT, FFD_NAV_UP, FFD_NAV_RIGHT,
                              FFD_NAV_DOWN, FFD_NAV_ACCEPT, FFD_NAV_BACK};
    unsigned result = 0;
    for (int i = 0; i < 6; ++i) {
        if (!(held & bits[i])) {
            state->hold_frames[i] = 0;
            continue;
        }
        unsigned frames = state->hold_frames[i]++;
        if (frames == 0 || (i < 4 && frames >= 12 && (frames - 12) % 3 == 0))
            result |= bits[i];
    }
    state->held_before = held;
    return result;
}
#endif
