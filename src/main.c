#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include "render.h"
#include "simulation.h"

#define SCALE 0.9
#define FPS 120
#define MAX_N 10000

int main() {

	//----------------------------------INIT
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("random_walk", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, 1920*SCALE, 1080*SCALE,
                                       SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!win || !ren) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
	Uint64 freq = SDL_GetPerformanceFrequency();
	Uint64 frame_ticks = freq / FPS;
	Uint64 next_frame = SDL_GetPerformanceCounter();


	init_myrand();

    int running = 1, n = 0, step_once = 0, paused = 0;

	double x[MAX_N], y[MAX_N];
	//--------------------------------LOOP
    while (running) {

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: running = 0; break;
                case SDLK_SPACE:  if (paused) step_once = 1; break;
                case SDLK_p: paused = !paused; break;
                }
            }
        }

		//--------------------------SIMULATION


		 if (!paused || step_once) {
            if (n < MAX_N) {
                rand_step(n,x,y);
                n++;
            }
            step_once = 0;
        }


		//-----------------------------PLOT
		plot(ren, n , x, y);

        SDL_RenderPresent(ren);
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

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
