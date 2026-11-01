#include "render.h"

#include <SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CURVE_W    2.0f    /* spessore della curva (pixel) */
#define TICK_LEN   6.0f    /* lunghezza delle stanghette (pixel) */
#define MAX_TICKS  32
#define CHUNK_SEG  8000

static const SDL_Color COL_CURVE = { 80, 200, 120, 255};
static const SDL_Color COL_HEAD  = {235, 235, 245, 255};
static const SDL_Color COL_TEXT  = {205, 205, 220, 255};
static const SDL_Color COL_FRAME = {110, 110, 130, 255};
static const SDL_Color COL_GRID  = { 42,  42,  56, 255};
static const SDL_Color COL_ZERO  = { 95,  95, 120, 255};

typedef struct { double min, max; } Range;
typedef struct { SDL_FRect r; Range x, y; } View;          /* rettangolo + range dati */
typedef struct { double v[MAX_TICKS]; int n; int decimals; double step; } Ticks;
typedef enum { ALIGN_START, ALIGN_CENTER, ALIGN_END } Align;

struct Plot {
    SDL_Renderer *ren;
    TTF_Font     *font;
    int           font_h;
    SDL_Vertex   *vtx;
    int          *idx;
    SDL_FPoint   *pts;
    int           pts_cap;
};

/* ------------------------------------------------------------ utilita' */

static void set_color(SDL_Renderer *r, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

static float vx(const View *v, double x)
{
    return (float)((double)v->r.x + (x - v->x.min) / (v->x.max - v->x.min) * (double)v->r.w);
}
static float vy(const View *v, double y)
{
    return (float)((double)(v->r.y + v->r.h) - (y - v->y.min) / (v->y.max - v->y.min) * (double)v->r.h);
}

/* ------------------------------------------------------------- testo */

static float align_k(Align a)
{
    return a == ALIGN_START ? 0.0f : (a == ALIGN_CENTER ? 0.5f : 1.0f);
}

static int text_w(const Plot *p, const char *s)
{
    int w = 0, h = 0;
    if (p->font && s) TTF_SizeUTF8(p->font, s, &w, &h);
    return w;
}

static void draw_text(Plot *p, const char *s, float x, float y,
                      Align ax, Align ay, double angle)
{
    if (!p->font || !s || !*s) return;

    SDL_Surface *surf = TTF_RenderUTF8_Blended(p->font, s, COL_TEXT);
    if (!surf) return;

    if (angle != 0.0) {
        SDL_Surface *rot = SDL_CreateRGBSurfaceWithFormat(0, surf->h, surf->w, 32,
                                                          surf->format->format);
        if (rot) {
            const Uint32 *src = (const Uint32 *)surf->pixels;
            Uint32 *dst = (Uint32 *)rot->pixels;
            const int sp = surf->pitch / 4, dp = rot->pitch / 4;
            for (int yy = 0; yy < surf->h; yy++)
                for (int xx = 0; xx < surf->w; xx++)
                    dst[(surf->w - 1 - xx) * dp + yy] = src[yy * sp + xx];
            SDL_FreeSurface(surf);
            surf = rot;
        }
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(p->ren, surf);
    const float w = (float)surf->w, h = (float)surf->h;
    SDL_FreeSurface(surf);
    if (!tex) return;

    SDL_FRect d = { roundf(x - w * align_k(ax)), roundf(y - h * align_k(ay)), w, h };
    SDL_RenderCopyF(p->ren, tex, NULL, &d);
    SDL_DestroyTexture(tex);
}

/* ------------------------------------------------------------- tick */

static double nice_num(double range, int round_it)
{
    double e = floor(log10(range));
    double f = range / pow(10.0, e);
    double nf;
    if (round_it) nf = f < 1.5 ? 1.0 : (f < 3.0 ? 2.0 : (f < 7.0 ? 5.0 : 10.0));
    else          nf = f <= 1.0 ? 1.0 : (f <= 2.0 ? 2.0 : (f <= 5.0 ? 5.0 : 10.0));
    return nf * pow(10.0, e);
}

static void make_ticks(Range rg, int target, Ticks *t)
{
    if (target < 2) target = 2;
    const double span = rg.max - rg.min;
    const double step = nice_num(nice_num(span, 0) / (double)(target - 1), 1);
    const double k0   = ceil(rg.min / step - 1e-9);

    t->n = 0;
    t->step = step;
    for (int k = 0; t->n < MAX_TICKS; k++) {
        double v = (k0 + (double)k) * step;
        if (v > rg.max + step * 1e-9) break;
        if (fabs(v) < step * 1e-9) v = 0.0;
        t->v[t->n++] = v;
    }
    t->decimals = step >= 1.0 ? 0 : (int)(-floor(log10(step) + 1e-9));
}

static void fmt_tick(char *buf, size_t sz, double v, const Ticks *t)
{
    double amax = t->n > 0 ? fmax(fabs(t->v[0]), fabs(t->v[t->n - 1])) : 0.0;
    if (amax >= 1e7 || t->step < 1e-4)
        snprintf(buf, sz, "%.3g", v);
    else
        snprintf(buf, sz, "%.*f", t->decimals, v);
}

/* ------------------------------------------------------------- range */

static void fix_range(Range *r)
{
    const double span  = r->max - r->min;
    const double scale = fmax(fabs(r->min), fabs(r->max));
    if (span <= 1e-12 * scale) {
        const double h = scale > 0.0 ? 0.1 * scale : 1.0;
        r->min -= h;
        r->max += h;
    }
}

static void data_ranges(int n, const double *x, const double *y, Range *rx, Range *ry)
{
    if (n < 1) {
        rx->min = ry->min = 0.0;
        rx->max = ry->max = 1.0;
        return;
    }
    rx->min = rx->max = x[0];
    ry->min = ry->max = y[0];
    for (int i = 1; i < n; i++) {
        if (x[i] < rx->min) rx->min = x[i];
        if (x[i] > rx->max) rx->max = x[i];
        if (y[i] < ry->min) ry->min = y[i];
        if (y[i] > ry->max) ry->max = y[i];
    }
    fix_range(rx);
    fix_range(ry);
    const double pad = 0.05 * (ry->max - ry->min);
    ry->min -= pad;
    ry->max += pad;
    rx->max += 0.02 * (rx->max - rx->min);
}

/* ------------------------------------------------------------- curva */

static void draw_polyline(Plot *p, const SDL_FPoint *pt, int npts, float w, SDL_Color c)
{
    const int nseg = npts - 1;
    const float hw = 0.5f * w;

    for (int s0 = 0; s0 < nseg; s0 += CHUNK_SEG) {
        int cnt = nseg - s0;
        if (cnt > CHUNK_SEG) cnt = CHUNK_SEG;

        for (int s = 0; s < cnt; s++) {
            const SDL_FPoint a = pt[s0 + s], b = pt[s0 + s + 1];
            const float dx = b.x - a.x, dy = b.y - a.y;
            const float len = sqrtf(dx * dx + dy * dy);
            float ux = 0.0f, uy = 0.0f, ext = 0.0f;
            if (len > 1e-6f) { ux = dx / len; uy = dy / len; ext = hw; }
            const float nx = -uy * hw, ny = ux * hw;
            const float x1 = a.x - ux * ext, y1 = a.y - uy * ext;
            const float x2 = b.x + ux * ext, y2 = b.y + uy * ext;

            SDL_Vertex *v = &p->vtx[4 * s];
            v[0].position.x = x1 + nx; v[0].position.y = y1 + ny;
            v[1].position.x = x1 - nx; v[1].position.y = y1 - ny;
            v[2].position.x = x2 + nx; v[2].position.y = y2 + ny;
            v[3].position.x = x2 - nx; v[3].position.y = y2 - ny;
            for (int k = 0; k < 4; k++) {
                v[k].color = c;
                v[k].tex_coord.x = 0.0f;
                v[k].tex_coord.y = 0.0f;
            }
        }
        SDL_RenderGeometry(p->ren, NULL, p->vtx, 4 * cnt, p->idx, 6 * cnt);
    }
}

static void draw_curve(Plot *p, const View *v, int n, const double *x, const double *y)
{
    if (n < 1) return;

    if (n > p->pts_cap) {
        int cap = n > 2 * p->pts_cap ? n : 2 * p->pts_cap;
        SDL_FPoint *tmp = realloc(p->pts, (size_t)cap * sizeof *tmp);
        if (!tmp) return;
        p->pts = tmp;
        p->pts_cap = cap;
    }

    int np = 0;
    int i = 0;
    while (i < n) {
        const float col = floorf(vx(v, x[i]));
        int imin = i, imax = i, last = i;
        int j = i + 1;
        for (; j < n && floorf(vx(v, x[j])) == col; j++) {
            if (y[j] < y[imin]) imin = j;
            if (y[j] > y[imax]) imax = j;
            last = j;
        }

        int k[4] = { i, imin, imax, last };
        for (int a = 1; a < 4; a++) {
            int key = k[a], b = a - 1;
            while (b >= 0 && k[b] > key) { k[b + 1] = k[b]; b--; }
            k[b + 1] = key;
        }
        for (int a = 0; a < 4; a++) {
            if (a > 0 && k[a] == k[a - 1]) continue;
            p->pts[np].x = vx(v, x[k[a]]);
            p->pts[np].y = vy(v, y[k[a]]);
            np++;
        }
        i = j;
    }

    if (np >= 2) draw_polyline(p, p->pts, np, CURVE_W, COL_CURVE);
}

/* ------------------------------------------------------------ API */

Plot *plot_create(SDL_Renderer *ren, const char *font_path, int font_px)
{
    Plot *p = calloc(1, sizeof *p);
    if (!p) return NULL;

    p->ren = ren;
    p->font_h = font_px;
    p->vtx = malloc((size_t)CHUNK_SEG * 4 * sizeof *p->vtx);
    p->idx = malloc((size_t)CHUNK_SEG * 6 * sizeof *p->idx);
    if (!p->vtx || !p->idx) { plot_destroy(p); return NULL; }

    for (int s = 0; s < CHUNK_SEG; s++) {
        int *q = &p->idx[6 * s];
        const int b = 4 * s;
        q[0] = b; q[1] = b + 1; q[2] = b + 2;
        q[3] = b + 2; q[4] = b + 1; q[5] = b + 3;
    }

    if (font_path) {
        if (TTF_Init() != 0) {
            fprintf(stderr, "plot: TTF_Init: %s\n", TTF_GetError());
        } else {
            p->font = TTF_OpenFont(font_path, font_px);
            if (p->font) {
                p->font_h = TTF_FontHeight(p->font);
            } else {
                fprintf(stderr, "plot: font '%s': %s (il grafico non avra' testo)\n",
                        font_path, TTF_GetError());
                TTF_Quit();
            }
        }
    }
    return p;
}

void plot_destroy(Plot *p)
{
    if (!p) return;
    if (p->font) {
        TTF_CloseFont(p->font);
        TTF_Quit();
    }
    free(p->vtx);
    free(p->idx);
    free(p->pts);
    free(p);
}

void plot_draw(Plot *p, int n, const double *x, const double *y, const char *title, const char *xlabel, const char *ylabel)
{
    int W = 0, H = 0;
    SDL_GetRendererOutputSize(p->ren, &W, &H);

    Range rx, ry;
    data_ranges(n, x, y, &rx, &ry);

    const float fh    = (float)p->font_h;
    const int   has_t = title  && *title;
    const int   has_x = xlabel && *xlabel;
    const int   has_y = ylabel && *ylabel;
    const float top   = has_t ? 2.0f * fh : 0.8f * fh;
    const float bot   = TICK_LEN + 4.0f + fh + (has_x ? fh + 10.0f : 0.0f) + 10.0f;
    const float right = 24.0f;
    const float h     = (float)H - top - bot;
    if (h < 60.0f || W < 200) return;

    Ticks ty;
    make_ticks(ry, (int)(h / (4.0f * fh)), &ty);
    int lw = 0;
    for (int i = 0; i < ty.n; i++) {
        char buf[32];
        fmt_tick(buf, sizeof buf, ty.v[i], &ty);
        const int w = text_w(p, buf);
        if (w > lw) lw = w;
    }
    const float left = (has_y ? fh + 12.0f : 8.0f) + (float)lw + TICK_LEN + 10.0f;

    View v = { { left, top, (float)W - left - right, h }, rx, ry };
    if (v.r.w < 60.0f) return;

    Ticks tx;
    make_ticks(rx, (int)(v.r.w / (7.0f * fh)), &tx);

    const SDL_FRect *r = &v.r;

    /* --- griglia--------- */
    set_color(p->ren, COL_GRID);
    for (int i = 0; i < tx.n; i++) {
        const float px = vx(&v, tx.v[i]);
        SDL_RenderDrawLineF(p->ren, px, r->y, px, r->y + r->h);
    }
    for (int i = 0; i < ty.n; i++) {
        const float py = vy(&v, ty.v[i]);
        SDL_RenderDrawLineF(p->ren, r->x, py, r->x + r->w, py);
    }
    set_color(p->ren, COL_ZERO);
    if (rx.min < 0.0 && rx.max > 0.0) {
        const float px = vx(&v, 0.0);
        SDL_RenderDrawLineF(p->ren, px, r->y, px, r->y + r->h);
    }
    if (ry.min < 0.0 && ry.max > 0.0) {
        const float py = vy(&v, 0.0);
        SDL_RenderDrawLineF(p->ren, r->x, py, r->x + r->w, py);
    }

    SDL_Rect clip = { (int)floorf(r->x), (int)floorf(r->y),
                      (int)ceilf(r->w) + 1, (int)ceilf(r->h) + 1 };
    SDL_RenderSetClipRect(p->ren, &clip);
    draw_curve(p, &v, n, x, y);
    SDL_RenderSetClipRect(p->ren, NULL);

    if (n >= 1) {
        const float hx = vx(&v, x[n - 1]), hy = vy(&v, y[n - 1]);
        const SDL_FRect d = { hx - 3.0f, hy - 3.0f, 6.0f, 6.0f };
        set_color(p->ren, COL_HEAD);
        SDL_RenderFillRectF(p->ren, &d);
    }

    set_color(p->ren, COL_FRAME);
    SDL_RenderDrawRectF(p->ren, r);


    char buf[32];
    for (int i = 0; i < tx.n; i++) {
        const float px = vx(&v, tx.v[i]);
        set_color(p->ren, COL_FRAME);
        SDL_RenderDrawLineF(p->ren, px, r->y + r->h, px, r->y + r->h + TICK_LEN);
        fmt_tick(buf, sizeof buf, tx.v[i], &tx);
        draw_text(p, buf, px, r->y + r->h + TICK_LEN + 4.0f, ALIGN_CENTER, ALIGN_START, 0.0);
    }
    for (int i = 0; i < ty.n; i++) {
        const float py = vy(&v, ty.v[i]);
        set_color(p->ren, COL_FRAME);
        SDL_RenderDrawLineF(p->ren, r->x - TICK_LEN, py, r->x, py);
        fmt_tick(buf, sizeof buf, ty.v[i], &ty);
        draw_text(p, buf, r->x - TICK_LEN - 4.0f, py, ALIGN_END, ALIGN_CENTER, 0.0);
    }


    if (has_t)
        draw_text(p, title, r->x + r->w / 2.0f, top / 2.0f, ALIGN_CENTER, ALIGN_CENTER, 0.0);
    if (has_x)
        draw_text(p, xlabel, r->x + r->w / 2.0f, r->y + r->h + TICK_LEN + 4.0f + fh + 6.0f,
                  ALIGN_CENTER, ALIGN_START, 0.0);
    if (has_y)
        draw_text(p, ylabel, 6.0f + fh / 2.0f, r->y + r->h / 2.0f,
                  ALIGN_CENTER, ALIGN_CENTER, -90.0);
}
