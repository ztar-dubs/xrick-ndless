/*
 * sysvid_nspire.c - Video layer for TI-Nspire CX CAS II
 *
 * Replaces the original sysvid.c which uses SDL2.
 * Uses SDL 1.2 (nSDL) with a 320x240 16-bit RGB565 surface.
 * The game framebuffer is 320x200 8-bit palettized, centered vertically.
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
 * sysvid_update
 *
 * Convert the 8-bit palettized framebuffer to RGB565 and blit to screen.
 * The game FB (320x200) is centered vertically on the Nspire screen (320x240).
 *
 * Simple pixel-by-pixel conversion with precomputed palette lookup.
 */
void sysvid_update(rect_t *rects)
{
    rect_t *rect;
    U16 x, y;
    U8 *src_row;
    U16 *dst_row;
    U16 pitch16;

    if (rects == NULL || screen == NULL)
        return;

    if (SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);

    pitch16 = screen->pitch / 2;  /* pitch in U16 units */

    rect = rects;
    while (rect) {
        U16 rx = rect->x;
        U16 ry = rect->y;
        U16 rw = rect->width;
        U16 rh = rect->height;

        /* Clamp to framebuffer bounds */
        if (rx + rw > fb_width) rw = fb_width - rx;
        if (ry + rh > fb_height) rh = fb_height - ry;

        for (y = ry; y < ry + rh; y++) {
            src_row = ((U8*)&fb) + y * fb_width + rx;
            dst_row = (U16*)screen->pixels + (y + Y_OFFSET) * pitch16 + rx;

            for (x = 0; x < rw; x++) {
                dst_row[x] = palette565[src_row[x]];
            }
        }

        rect = rect->next;
    }

    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);

    SDL_Flip(screen);
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
