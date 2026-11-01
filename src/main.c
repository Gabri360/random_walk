#include <SDL.h>
#include <stdio.h>
#include "render.h"

#define N 1000
#define SCALE 0.9


typedef struct { double xmin, xmax, ymin, ymax; } View;


static float map_x(const View *v, SDL_FRect r, double x) {
    return (float)(r.x + (x - v->xmin) / (v->xmax - v->xmin) * r.w);
}
static float map_y(const View *v, SDL_FRect r, double y) {
    return (float)(r.y + r.h - (y - v->ymin) / (v->ymax - v->ymin) * r.h);
}

int main() {

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("random_walk", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, 1920*SCALE, 1080*SCALE,
                                       SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                          SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!win || !ren) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    /* dati di prova: y = 2x + 1 su [-5, 5] */
    double xs[N], ys[N];
    for (int i = 0; i < N; i++) {
        xs[i] = -5.0 + 10.0 * i / (N - 1);
        ys[i] = xs[i] * xs[i] + 1.0;
    }

    /* autoscale con un po' di margine */
    View v = { xs[0], xs[N - 1], ys[0], ys[0] };
    for (int i = 0; i < N; i++) {
        if (ys[i] < v.ymin) v.ymin = ys[i];
        if (ys[i] > v.ymax) v.ymax = ys[i];
    }
    double pad = 0.05 * (v.ymax - v.ymin);
    v.ymin -= pad; v.ymax += pad;

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }

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
                map_x(&v, plot, xs[i - 1]), map_y(&v, plot, ys[i - 1]),
                map_x(&v, plot, xs[i]),     map_y(&v, plot, ys[i]), 12.0f, linea);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
