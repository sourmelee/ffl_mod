/* FFLmod geometry companion, independent of Android/Vita/Switch graphics APIs. */
#ifndef FFD_DISPLAY_TRANSFORM_H
#define FFD_DISPLAY_TRANSFORM_H
#include "../src/display_geometry.h"

typedef struct { float x, y; } FfdPoint;

static FfdPoint ffd_game_to_surface(float gx, float gy, float surface_w, float surface_h)
{
    DisplayGeometry g = display_geometry(surface_w, surface_h);
    FfdPoint p = {(gx + g.x) * g.scale, (gy + g.y) * g.scale};
    return p;
}

static FfdPoint ffd_surface_to_game(float px, float py, float surface_w, float surface_h)
{
    DisplayGeometry g = display_geometry(surface_w, surface_h);
    FfdPoint p = {px / g.scale - g.x, py / g.scale - g.y};
    return p;
}
#endif
