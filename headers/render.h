#ifndef RENDER_H
#define RENDER_H


#include <SDL.h>

typedef struct Plot Plot;

/* Crea il "contesto" del grafico (font, buffer di vertici).
 * font_path puo' essere NULL o non valido: il grafico funziona lo stesso,
 * solo senza testo. Va distrutto con plot_destroy(). */
Plot *plot_create(SDL_Renderer *ren, const char *font_path, int font_px);
void  plot_destroy(Plot *p);

/* Disegna n punti (x crescente) con assi, tick, griglia ed etichette.
 * NON fa SDL_RenderClear ne' SDL_RenderPresent: li fa il chiamante.
 * title, xlabel, ylabel possono essere NULL. */
void  plot_draw(Plot *p, int n, const double *x, const double *y,
                const char *title, const char *xlabel, const char *ylabel);


#endif
