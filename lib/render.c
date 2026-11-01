#include "render.h"
#include <math.h>

void thick_line(SDL_Renderer *r, float x1, float y1, float x2, float y2,
                       float w, SDL_Color c)
{
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) return;

    float nx = -dy / len * w * 0.5f;
    float ny =  dx / len * w * 0.5f;

    SDL_Vertex v[4] = {
        { { x1 + nx, y1 + ny }, c, { 0, 0 } },
        { { x1 - nx, y1 - ny }, c, { 0, 0 } },
        { { x2 + nx, y2 + ny }, c, { 0, 0 } },
        { { x2 - nx, y2 - ny }, c, { 0, 0 } },
    };
    const int idx[6] = { 0, 1, 2, 2, 1, 3 };
    SDL_RenderGeometry(r, NULL, v, 4, idx, 6);
}
