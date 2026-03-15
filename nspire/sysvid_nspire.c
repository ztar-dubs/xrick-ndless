/*
 * sysvid_nspire.c - Video layer for TI-Nspire CX CAS II
 *
 * Replaces the original sysvid.c which uses SDL2.
 * Uses SDL 1.2 (nSDL) with a 320x240 16-bit RGB565 surface.
 * The game framebuffer is 320x200 8-bit palettized, centered vertically.
 *
 * Optimizations:
 * - Precomputed RGB565 palette lookup table
 * - Unrolled conversion loop (4 pixels/iteration, U32 writes)
 * - Dirty rect merging to avoid redundant pixel conversion
 * - SDL_UpdateRects instead of SDL_Flip for partial updates
 */

#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>

#include "sysvid.h"
#include "sysarg.h"
#include "debug.h"
#include "fb.h"
#include "img.h"

/* Nspire screen dimensions */
#define NSPIRE_WIDTH  320
#define NSPIRE_HEIGHT 240

/* Vertical offset to center 320x200 in 320x240 */
#define Y_OFFSET 20

/* Max dirty rects to track before falling back to full screen update */
#define MAX_DIRTY_RECTS 32

rect_t SCREENRECT = {0, 0, FB_WIDTH, FB_HEIGHT, NULL};

static U16 paln;
static SDL_Color pals[256], pald[256];
static SDL_Surface *screen = NULL;
static U8 gamma_val;
static U16 fb_width, fb_height;

/* Precomputed RGB565 palette for fast blitting */
static U16 palette565[256];

/*
 * Convert RGB888 to RGB565
 */
static U16 rgb888_to_rgb565(U8 r, U8 g, U8 b)
{
    return ((U16)(r >> 3) << 11) | ((U16)(g >> 2) << 5) | (U16)(b >> 3);
}

/*
 * Rebuild the RGB565 palette from pald[]
 */
static void rebuild_palette565(void)
{
    U16 i;
    for (i = 0; i < paln; i++) {
        palette565[i] = rgb888_to_rgb565(pald[i].r, pald[i].g, pald[i].b);
    }
}

void sysvid_setPaletteFromImg(img_t *img)
{
    U16 i;

    if ((paln = img->ncolors) == 0) return;

    for (i = 0; i < paln; ++i) {
        pals[i].r = img->colors[i].r;
        pals[i].g = img->colors[i].g;
        pals[i].b = img->colors[i].b;
    }

    sysvid_setDisplayPalette();
}

void sysvid_setPaletteFromRGB(U8 *r, U8 *g, U8 *b, U16 n)
{
    U16 i;

    if ((paln = n) == 0) return;

    for (i = 0; i < paln; ++i) {
        pals[i].r = r[i];
        pals[i].g = g[i];
        pals[i].b = b[i];
    }

    sysvid_setDisplayPalette();
}

void sysvid_setDisplayPalette(void)
{
    U16 i;

    if (paln == 0) return;

    for (i = 0; i < paln; i++) {
        pald[i].r = pals[i].r * gamma_val / 255;
        pald[i].g = pals[i].g * gamma_val / 255;
        pald[i].b = pals[i].b * gamma_val / 255;
    }

    rebuild_palette565();
}

void sysvid_init(U16 width, U16 height)
{
    fb_width = width;
    fb_height = height;

    /* Initialize SDL 1.2 video */
    screen = SDL_SetVideoMode(NSPIRE_WIDTH, NSPIRE_HEIGHT, 16, SDL_SWSURFACE);
    if (!screen) {
        exit(1);
    }

    /* Clear screen to black */
    SDL_FillRect(screen, NULL, 0);
    SDL_Flip(screen);

    gamma_val = 255;
}

void sysvid_shutdown(void)
{
    screen = NULL;
}

/*
 * Convert one scanline from 8-bit palettized to RGB565.
 * Unrolled: processes 4 pixels per iteration using U32 writes.
 * ARM926EJ-S handles aligned 32-bit writes in one cycle.
 */
static void convert_scanline(const U8 *src, U16 *dst, U16 width)
{
    U16 w4 = width >> 2;  /* number of 4-pixel groups */
    U16 rem = width & 3;  /* remaining pixels */
    U32 *dst32 = (U32 *)dst;

    while (w4--) {
        /* Pack 2 RGB565 pixels into one U32 (little-endian ARM) */
        dst32[0] = (U32)palette565[src[0]] | ((U32)palette565[src[1]] << 16);
        dst32[1] = (U32)palette565[src[2]] | ((U32)palette565[src[3]] << 16);
        src += 4;
        dst32 += 2;
    }

    /* Handle remaining 1-3 pixels */
    dst = (U16 *)dst32;
    while (rem--) {
        *dst++ = palette565[*src++];
    }
}

/*
 * Merge overlapping/adjacent dirty rects to reduce redundant blitting.
 * Takes a linked list of game rects, outputs merged SDL_Rects.
 * Returns the number of merged rects.
 */
static int merge_rects(rect_t *rects, SDL_Rect *out, int max)
{
    int n = 0;
    rect_t *rect;

    /* Collect rects, clamping to FB bounds */
    rect = rects;
    while (rect && n < max) {
        U16 rx = rect->x;
        U16 ry = rect->y;
        U16 rw = rect->width;
        U16 rh = rect->height;

        if (rx + rw > fb_width) rw = fb_width - rx;
        if (ry + rh > fb_height) rh = fb_height - ry;

        if (rw > 0 && rh > 0) {
            out[n].x = rx;
            out[n].y = ry;
            out[n].w = rw;
            out[n].h = rh;
            n++;
        }
        rect = rect->next;
    }

    /* If too many rects, fall back to full screen */
    if (rect != NULL) {
        out[0].x = 0;
        out[0].y = 0;
        out[0].w = fb_width;
        out[0].h = fb_height;
        return 1;
    }

    if (n <= 1) return n;

    /* Merge overlapping/adjacent rects (simple greedy pass) */
    for (int i = 0; i < n; i++) {
        if (out[i].w == 0) continue;
        int merged = 1;
        while (merged) {
            merged = 0;
            for (int j = i + 1; j < n; j++) {
                if (out[j].w == 0) continue;

                /* Check if rects overlap or are adjacent (within 8px tolerance) */
                S16 ax1 = out[i].x, ay1 = out[i].y;
                S16 ax2 = ax1 + out[i].w, ay2 = ay1 + out[i].h;
                S16 bx1 = out[j].x, by1 = out[j].y;
                S16 bx2 = bx1 + out[j].w, by2 = by1 + out[j].h;

                if (ax1 <= bx2 + 8 && ax2 + 8 >= bx1 &&
                    ay1 <= by2 + 8 && ay2 + 8 >= by1) {
                    /* Merge: bounding box of both */
                    S16 nx1 = ax1 < bx1 ? ax1 : bx1;
                    S16 ny1 = ay1 < by1 ? ay1 : by1;
                    S16 nx2 = ax2 > bx2 ? ax2 : bx2;
                    S16 ny2 = ay2 > by2 ? ay2 : by2;
                    out[i].x = nx1;
                    out[i].y = ny1;
                    out[i].w = nx2 - nx1;
                    out[i].h = ny2 - ny1;
                    out[j].w = 0; /* mark as consumed */
                    merged = 1;
                }
            }
        }
    }

    /* Compact: remove consumed rects */
    int out_n = 0;
    for (int i = 0; i < n; i++) {
        if (out[i].w > 0) {
            out[out_n++] = out[i];
        }
    }

    return out_n;
}

/*
 * sysvid_update
 *
 * Convert the 8-bit palettized framebuffer to RGB565 and blit to screen.
 * The game FB (320x200) is centered vertically on the Nspire screen (320x240).
 *
 * Optimizations vs naive approach:
 * 1. Dirty rects are merged to avoid converting overlapping pixels twice
 * 2. Conversion loop is unrolled (4 pixels/iter, 32-bit writes)
 * 3. Only the dirty SDL rects are sent to the LCD (SDL_UpdateRects)
 */
void sysvid_update(rect_t *rects)
{
    SDL_Rect sdl_rects[MAX_DIRTY_RECTS];
    int nrects, i;

    if (rects == NULL || screen == NULL)
        return;

    /* Merge overlapping dirty rects */
    nrects = merge_rects(rects, sdl_rects, MAX_DIRTY_RECTS);
    if (nrects == 0)
        return;

    if (SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);

    {
        U16 pitch16 = screen->pitch / 2;

        for (i = 0; i < nrects; i++) {
            U16 rx = sdl_rects[i].x;
            U16 ry = sdl_rects[i].y;
            U16 rw = sdl_rects[i].w;
            U16 rh = sdl_rects[i].h;
            U16 y;

            for (y = ry; y < ry + rh; y++) {
                const U8 *src = ((const U8 *)&fb) + y * fb_width + rx;
                U16 *dst = (U16 *)screen->pixels + (y + Y_OFFSET) * pitch16 + rx;
                convert_scanline(src, dst, rw);
            }

            /* Adjust SDL rect for Y_OFFSET (screen coordinates) */
            sdl_rects[i].y += Y_OFFSET;
        }
    }

    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);

    /* Only update the dirty regions on the LCD */
    SDL_UpdateRects(screen, nrects, sdl_rects);
}

void sysvid_zoom(S8 z)
{
    (void)z;
}

void sysvid_toggleFullscreen(void)
{
}

void sysvid_setGamma(U8 g)
{
    gamma_val = g;
    sysvid_setDisplayPalette();
}
