/*
 * Final Fantasy Dimensions portable game-aware menu navigation.
 * Copyright (c) 2026 fflmod contributors. MIT License.
 *
 * Platform-neutral portion of FFLmod's MenuWidget cursor logic.
 * Caller enumerates visible, active, touchable engine items per frame.
 * Coordinates are 640x384 logical game units, not physical screen pixels.
 */
#ifndef FFD_PORTABLE_NAV_H
#define FFD_PORTABLE_NAV_H
#include <stdint.h>
#include <stddef.h>

#define FFD_NAV_MAX_CANDIDATES 160
#define FFD_NAV_LEFT   (1u << 0)
#define FFD_NAV_UP     (1u << 1)
#define FFD_NAV_RIGHT  (1u << 2)
#define FFD_NAV_DOWN   (1u << 3)
#define FFD_NAV_ACCEPT (1u << 4)
#define FFD_NAV_BACK   (1u << 5)
#define FFD_NAV_DIRS (FFD_NAV_LEFT | FFD_NAV_UP | FFD_NAV_RIGHT | FFD_NAV_DOWN)

/* Types match FFLmod: 0 ordinary, 1 list/grid, 2 MenuClass, 3 item list.
 * item/widget are opaque identity tokens provided by the platform adapter. */
typedef struct {
    uintptr_t item, widget;
    int sub, list, id;
    float x, y, w, h;
} FfdNavCandidate;
typedef struct {
    uintptr_t item, widget;
    int sub, id;
    float x, y;
    int has_selection;
} FfdNavState;
typedef struct {
    int selected_index; /* -1 = no candidates */
    int activated;
    float touch_x, touch_y;
} FfdNavResult;

static float ffd_nav_absf(float v) { return v < 0.0f ? -v : v; }
static float ffd_nav_cx(const FfdNavCandidate *c) { return c->x + c->w * 0.5f; }
static float ffd_nav_cy(const FfdNavCandidate *c) { return c->y + c->h * 0.5f; }

/* Identical scoring/wrapping policy to FFLmod's nav() in src/fflmod.c. */
static int ffd_nav_move(const FfdNavCandidate *c, int count, int from, unsigned dir)
{
    if (!c || count <= 1 || from < 0 || from >= count ||
        (dir != FFD_NAV_LEFT && dir != FFD_NAV_RIGHT &&
         dir != FFD_NAV_UP && dir != FFD_NAV_DOWN)) return from;
    const FfdNavCandidate *origin = &c[from];
    const float fx = ffd_nav_cx(origin), fy = ffd_nav_cy(origin);
    float best_score = 1e30f;
    int best = -1;
    for (int i = 0; i < count; i++) {
        if (i == from || c[i].w <= 0.0f || c[i].h <= 0.0f) continue;
        float dx = ffd_nav_cx(&c[i]) - fx;
        float dy = ffd_nav_cy(&c[i]) - fy;
        float primary = 0.0f, secondary = 0.0f;
        switch (dir) {
            case FFD_NAV_LEFT:  primary = -dx; secondary = dy; break;
            case FFD_NAV_RIGHT: primary = dx;  secondary = dy; break;
            case FFD_NAV_UP:    primary = -dy; secondary = dx; break;
            case FFD_NAV_DOWN:  primary = dy;  secondary = dx; break;
        }
        if (primary < 2.0f) continue;
        float score = primary + 2.5f * ffd_nav_absf(secondary);
        int overlap = (dir == FFD_NAV_LEFT || dir == FFD_NAV_RIGHT)
            ? (c[i].y < origin->y + origin->h - 2.0f && c[i].y + c[i].h > origin->y + 2.0f)
            : (c[i].x < origin->x + origin->w - 2.0f && c[i].x + c[i].w > origin->x + 2.0f);
        if (overlap) score -= 1e6f;
        if (score < best_score) { best_score = score; best = i; }
    }
    if (best >= 0) return best;

    best_score = 1e30f;
    for (int i = 0; i < count; i++) {
        if (i == from || c[i].w <= 0.0f || c[i].h <= 0.0f) continue;
        float cx = ffd_nav_cx(&c[i]), cy = ffd_nav_cy(&c[i]);
        float along = 0.0f, secondary = 0.0f;
        switch (dir) {
            case FFD_NAV_LEFT:  along = -cx; secondary = cy - fy; break;
            case FFD_NAV_RIGHT: along = cx;  secondary = cy - fy; break;
            case FFD_NAV_UP:    along = -cy; secondary = cx - fx; break;
            case FFD_NAV_DOWN:  along = cy;  secondary = cx - fx; break;
        }
        float score = along + 2.5f * ffd_nav_absf(secondary);
        if (score < best_score) { best_score = score; best = i; }
    }
    return best >= 0 ? best : from;
}

/* Largest initial grid takes precedence, then topmost MenuClass button. */
static int ffd_nav_default(const FfdNavCandidate *c, int count, uintptr_t widget)
{
    if (!c || count <= 0) return -1;
    int best = -1, best_count = 0;
    for (int i = 0; i < count; i++) {
        if ((widget && c[i].widget != widget) || c[i].list != 1 || c[i].sub != 0 ||
            c[i].w <= 0.0f || c[i].h <= 0.0f) continue;
        int total = 0;
        for (int j = 0; j < count; j++) if (c[j].item == c[i].item) total++;
        if (total > best_count) { best = i; best_count = total; }
    }
    if (best >= 0) return best;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < count; i++) {
            if ((widget && c[i].widget != widget) || c[i].w <= 0.0f || c[i].h <= 0.0f ||
                (pass == 0 && c[i].list != 2)) continue;
            if (best < 0 || c[i].y < c[best].y - 4.0f ||
                (ffd_nav_absf(c[i].y - c[best].y) <= 4.0f && c[i].x < c[best].x)) best = i;
        }
        if (best >= 0) break;
    }
    return best;
}

static int ffd_nav_find(const FfdNavState *state, const FfdNavCandidate *c, int count)
{
    if (!state || !state->has_selection || !c) return -1;
    for (int i = 0; i < count; i++)
        if (c[i].item == state->item && c[i].widget == state->widget &&
            c[i].sub == state->sub && c[i].id == state->id) return i;
    if (state->id != 0)
        for (int i = 0; i < count; i++)
            if (c[i].id == state->id && c[i].widget == state->widget) return i;
    return -1;
}

/* Triggers are debounced/repeated by the console adapter, which handles input
 * ownership, touch timing and the engine-specific widget collection. */
static FfdNavResult ffd_nav_step(FfdNavState *state, const FfdNavCandidate *c,
                                 int count, uintptr_t preferred_widget, unsigned triggers)
{
    FfdNavResult out = {-1, 0, 0.0f, 0.0f};
    if (!state || !c || count <= 0) {
        if (state) state->has_selection = 0;
        return out;
    }
    if (count > FFD_NAV_MAX_CANDIDATES) count = FFD_NAV_MAX_CANDIDATES;
    int selected = ffd_nav_find(state, c, count);
    if (selected < 0) selected = ffd_nav_default(c, count, preferred_widget);
    if (selected < 0) { state->has_selection = 0; return out; }
    const unsigned dirs[] = {FFD_NAV_LEFT, FFD_NAV_RIGHT, FFD_NAV_UP, FFD_NAV_DOWN};
    for (unsigned i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
        if (triggers & dirs[i]) { selected = ffd_nav_move(c, count, selected, dirs[i]); break; }
    }
    const FfdNavCandidate *picked = &c[selected];
    state->item = picked->item;
    state->widget = picked->widget;
    state->sub = picked->sub;
    state->id = picked->id;
    state->x = ffd_nav_cx(picked);
    state->y = ffd_nav_cy(picked);
    state->has_selection = 1;
    out.selected_index = selected;
    out.activated = !!(triggers & FFD_NAV_ACCEPT);
    out.touch_x = state->x;
    out.touch_y = state->y;
    return out;
}
#endif
