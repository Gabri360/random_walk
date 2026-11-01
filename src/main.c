#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "render.h"
#include "simulation.h"
#include "const.h"


#define SCALE 0.9
#define FPS 60
#define MAX_STEPS_PER_FRAME 4096
#define FONT_FILE "assets/DejaVuSansMono.ttf"
#define FONT_PX 14

static double t[MAX_N], d[MAX_N], m[MAX_N];
static double x[DIM][MAX_N];

int main(int argc, char *argv[]) {

	//----------------------------------INIT
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_Window *win = SDL_CreateWindow("random_walk", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, 1920*SCALE, 1080*SCALE,
                                       SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!win || !ren) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

	char font_path[1024];
    char *base = SDL_GetBasePath();
    snprintf(font_path, sizeof font_path, "%s%s", base ? base : "", FONT_FILE);
    SDL_free(base);
    Plot *plot = plot_create(ren, font_path, FONT_PX);

	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 frame_ticks = freq / FPS;
	Uint64 next_frame = SDL_GetPerformanceCounter();


	uint64_t seed = argc > 1 ? strtoull(argv[1], NULL, 10) : (uint64_t)time(NULL);
    init_myrand(seed);

    int running = 1, n = 0, step_once = 0, paused = 0;
	int steps_per_frame = 1;

	//--------------------------------LOOP
    while (running) {

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: running = 0; break;
                case SDLK_SPACE:  if (paused) step_once = 1; break;
                case SDLK_p:      paused = !paused; break;
                case SDLK_r:      n = 0; break;
                case SDLK_UP:
                    if (steps_per_frame < MAX_STEPS_PER_FRAME) steps_per_frame *= 2;
                    break;
                case SDLK_DOWN:
                    if (steps_per_frame > 1) steps_per_frame /= 2;
                    break;
                }
            }
        }

		//--------------------------SIMULATION


		int steps = paused ? (step_once ? 1 : 0) : steps_per_frame;
        step_once = 0;
        for (int s = 0; s < steps && n < MAX_N; s++) {
            rand_step(n, t, x);
			comp_dist(n, x, d);
            comp_mean(n, d, m);
            n++;
        }



		//-----------------------------PLOT
        char title[96];
        snprintf(title, sizeof title, "Random walk  |  n = %d  |  %d passi/frame%s",
                 n, steps_per_frame, paused ? "  |  PAUSA" : "");

        SDL_SetRenderDrawColor(ren, 20, 20, 28, 255);
        SDL_RenderClear(ren);
        plot_draw(plot, n, t, d, title, "t", "d");
        SDL_RenderPresent(ren);

        //-----------------------------FPS
        next_frame += frame_ticks;
        Uint64 now = SDL_GetPerformanceCounter();

        if (now < next_frame) {
            double ms = (double)(next_frame - now) * 1000.0 / (double)freq;
            if (ms > 1.0) SDL_Delay((Uint32)(ms - 1.0));
            while (SDL_GetPerformanceCounter() < next_frame) {}
        } else if (now - next_frame > frame_ticks * 5) {
            next_frame = now;
        }
	}

	plot_destroy(plot);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
