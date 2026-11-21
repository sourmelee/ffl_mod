/* Display math shared by the renderer and host tests. No platform dependencies. */
#ifndef FFLMOD_DISPLAY_GEOMETRY_H
#define FFLMOD_DISPLAY_GEOMETRY_H
typedef struct { float scale, x, y; int active; } DisplayGeometry;
typedef struct { float x, y, w, h; } DisplayRect;

static DisplayGeometry display_geometry(float width, float height)
{
    DisplayGeometry r = { 1.0f, 0.0f, 0.0f, 0 };
    /* Keep portrait/square layouts on the game's original path. */
    if (!(width > 0.0f && height > 0.0f) || width > 32768.0f || height > 32768.0f ||
        width * 3.0f + 1.0f < height * 4.0f) return r;
    float sx = width / 640.0f, sy = height / 384.0f;
    r.scale = sx < sy ? sx : sy;
    float logicalW = width / r.scale, logicalH = height / r.scale;
    float roundedW = (float)(int)(logicalW + 0.5f), roundedH = (float)(int)(logicalH + 0.5f);
    if (logicalW - roundedW < 0.001f && roundedW - logicalW < 0.001f) logicalW = roundedW;
    if (logicalH - roundedH < 0.001f && roundedH - logicalH < 0.001f) logicalH = roundedH;
    r.x = (logicalW - 640.0f) * 0.5f;
    r.y = (logicalH - 384.0f) * 0.5f;
    if (r.x < 0.001f) r.x = 0.0f;
    if (r.y < 0.001f) r.y = 0.0f;
    r.active = r.x > 0.0f || r.y > 0.0f;
    return r;
}

static DisplayRect display_expand_image(DisplayRect r, float ex, float ey)
{
    int fullX = r.x <= 0.5f && r.x + r.w >= 639.5f;
    int fullY = r.y <= 0.5f && r.y + r.h >= 383.5f;
    if (ex > 0.0f) {
        if (fullX) {
            float scale = (640.0f + 2.0f * ex) / 640.0f;
            r.x = 320.0f + (r.x - 320.0f) * scale; r.w *= scale;
        } else if (fullY && (r.x <= 0.5f || r.x + r.w >= 639.5f)) {
            if (r.x <= 0.5f) r.x -= ex;
            r.w += ex;
        }
    }
    if (ey > 0.0f) {
        if (fullY) {
            float scale = (384.0f + 2.0f * ey) / 384.0f;
            r.y = 192.0f + (r.y - 192.0f) * scale; r.h *= scale;
        } else if (fullX && (r.y <= 0.5f || r.y + r.h >= 383.5f)) {
            if (r.y <= 0.5f) r.y -= ey;
            r.h += ey;
        }
    }
    return r;
}

static void display_expand_fill(int *x, int *y, int *w, int *h, float ex, float ey)
{
    int fullX = *x <= 0 && *x + *w >= 640;
    int fullY = *y <= 0 && *y + *h >= 384;
    int px = ex > 0.0f ? (int)(ex + 1.0f) : 0;
    int py = ey > 0.0f ? (int)(ey + 1.0f) : 0;
    if (px) {
        if (fullX) { *x -= px; *w += 2 * px; }
        else if (fullY && (*x <= 0 || *x + *w >= 640)) {
            if (*x <= 0) *x -= px;
            *w += px;
        }
    }
    if (py) {
        if (fullY) { *y -= py; *h += 2 * py; }
        else if (fullX && (*y <= 0 || *y + *h >= 384)) {
            if (*y <= 0) *y -= py;
            *h += py;
        }
    }
}
#endif
