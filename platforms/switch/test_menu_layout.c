/* Host-only test: cc -std=c11 -Wall -Wextra -Werror -I. \
 * platforms/switch/test_menu_layout.c -o /tmp/ffd-switch-nav-test
 * /tmp/ffd-switch-nav-test
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ffd_switch_menu_layout.h"

typedef struct {
    uintptr_t next;
    uintptr_t unused;
    uintptr_t item;
} MockNode;

static void put_ptr(unsigned char *buf, size_t off, uintptr_t value)
{
    memcpy(buf + off, &value, sizeof(value));
}
static void put_float(unsigned char *buf, size_t off, float value)
{
    memcpy(buf + off, &value, sizeof(value));
}
static void setup_item(unsigned char *p, float x, float y)
{
    memset(p, 0, 0x60);
    p[FFD_SWITCH_ITEM_ACTIVE] = 1;
    p[FFD_SWITCH_ITEM_VISIBLE] = 1;
    p[FFD_SWITCH_ITEM_TOUCHABLE] = 1;
    put_float(p, FFD_SWITCH_ITEM_X, x);
    put_float(p, FFD_SWITCH_ITEM_Y, y);
    put_float(p, FFD_SWITCH_ITEM_W, 80.0f);
    put_float(p, FFD_SWITCH_ITEM_H, 30.0f);
}
int main(void)
{
    unsigned char widget[0x50] = {0};
    unsigned char a[0x60], b[0x60], hidden[0x60];
    setup_item(a, 10.0f, 10.0f);
    setup_item(b, 10.0f, 50.0f);
    setup_item(hidden, 150.0f, 10.0f);
    hidden[FFD_SWITCH_ITEM_VISIBLE] = 0;
    MockNode nodes[3] = {
        {(uintptr_t)&nodes[1], 0, (uintptr_t)a},
        {(uintptr_t)&nodes[2], 0, (uintptr_t)b},
        {0, 0, (uintptr_t)hidden}
    };
    put_ptr(widget, FFD_SWITCH_WIDGET_HEAD, (uintptr_t)&nodes[0]);
    FfdNavCandidate candidates[FFD_NAV_MAX_CANDIDATES] = {{0}};
    int count = ffd_switch_collect_menu_items(widget, candidates, 160);
    assert(count == 2);
    assert(candidates[0].item == (uintptr_t)a);
    assert(candidates[1].widget == (uintptr_t)widget);
    assert(candidates[1].y == 50.0f);
    FfdNavState state = {0};
    FfdNavResult r = ffd_nav_step(&state, candidates, count, (uintptr_t)widget, 0);
    assert(r.selected_index == 0 && !r.activated);
    r = ffd_nav_step(&state, candidates, count, (uintptr_t)widget, FFD_NAV_DOWN);
    assert(r.selected_index == 1 && r.touch_y == 65.0f);
    r = ffd_nav_step(&state, candidates, count, (uintptr_t)widget, FFD_NAV_DOWN);
    assert(r.selected_index == 0); /* wrap */
    r = ffd_nav_step(&state, candidates, count, (uintptr_t)widget, FFD_NAV_ACCEPT);
    assert(r.activated && r.touch_y == 25.0f);
    b[FFD_SWITCH_ITEM_TOUCHABLE] = 0;
    assert(ffd_switch_collect_menu_items(widget, candidates, 160) == 1);

    FfdSwitchNavRepeat rep = {0};
    assert(ffd_switch_nav_triggers(&rep, FFD_NAV_DOWN) == FFD_NAV_DOWN);
    for (int frame = 1; frame < 12; ++frame)
        assert(ffd_switch_nav_triggers(&rep, FFD_NAV_DOWN) == 0);
    assert(ffd_switch_nav_triggers(&rep, FFD_NAV_DOWN) == FFD_NAV_DOWN);
    for (int frame = 0; frame < 2; ++frame)
        assert(ffd_switch_nav_triggers(&rep, FFD_NAV_DOWN) == 0);
    assert(ffd_switch_nav_triggers(&rep, FFD_NAV_DOWN) == FFD_NAV_DOWN);
    assert(ffd_switch_nav_triggers(&rep, 0) == 0);
    assert(ffd_switch_nav_triggers(&rep, FFD_NAV_DOWN) == FFD_NAV_DOWN);
    puts("Switch MenuWidget layout and navigation: PASS");
    return 0;
}
