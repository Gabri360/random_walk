#include "render.h"
#include <math.h>


typedef struct { double xmin, xmax, ymin, ymax; } View;


static float map_x(const View *v, SDL_FRect r, double x) {
    return (float)(r.x + (x - v->xmin) / (v->xmax - v->xmin) * r.w);
}
static float map_y(const View *v, SDL_FRect r, double y) {
    return (float)(r.y + r.h - (y - v->ymin) / (v->ymax - v->ymin) * r.h);
}

static void thick_line(SDL_Renderer *r, float x1, float y1, float x2, float y2,
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


void plot(SDL_Renderer *ren, int N, double x[], double y[])
{
	View v = { x[0], x[N - 1], y[0], y[0] };

	for (int i = 0; i < N; i++) {
		if (y[i] < v.ymin) v.ymin = y[i];
		if (y[i] > v.ymax) v.ymax = y[i];
	}
	double pad = 0.05 * (v.ymax - v.ymin);
	v.ymin -= pad; v.ymax += pad;
	v.xmin -= pad; v.xmax += pad;

	int w, h;
    SDL_GetRendererOutputSize(ren, &w, &h);
    const float m = 50.0f;
    SDL_FRect plot = { m, m, w - 2 * m, h - 2 * m };

    SDL_SetRenderDrawColor(ren, 20, 20, 28, 255);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 90, 90, 110, 255);
    SDL_RenderDrawRectF(ren, &plot);

    SDL_SetRenderDrawColor(ren, 160, 160, 180, 255);
    if (v.ymin < 0 && v.ymax > 0) {
        float y0 = map_y(&v, plot, 0);
        SDL_RenderDrawLineF(ren, plot.x, y0, plot.x + plot.w, y0);
    }
    if (v.xmin < 0 && v.xmax > 0) {
        float x0 = map_x(&v, plot, 0);
        SDL_RenderDrawLineF(ren, x0, plot.y, x0, plot.y + plot.h);
    }

    SDL_Color linea = {80, 200, 120, 255};
    for (int i = 1; i < N; i++) {
        thick_line(ren,
				   map_x(&v, plot, x[i - 1]), map_y(&v, plot, y[i - 1]),
				   map_x(&v, plot, x[i]),     map_y(&v, plot, y[i]), 12.0f, linea);
    }
}
