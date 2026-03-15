/*
 * SDL2-to-SDL1.2 compatibility header for TI-Nspire (nSDL)
 *
 * xrick uses SDL2. nSDL is SDL 1.2.
 * This header includes the real SDL 1.2 headers and provides
 * compatibility defines/types for the SDL2 features used by xrick.
 */

#ifndef NSPIRE_SDL2_COMPAT_H
#define NSPIRE_SDL2_COMPAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include the real nSDL (SDL 1.2) header */
#include <SDL/SDL.h>

/* ---- Types that exist in SDL2 but not SDL1.2 ---- */
typedef void SDL_Window;
typedef void SDL_Renderer;
typedef void SDL_Texture;

/* 64-bit types */
#ifndef Uint64
typedef unsigned long long Uint64;
#endif
#ifndef Sint64
typedef long long Sint64;
#endif

/* ---- SDL_SCANCODE compatibility ---- */
/* xrick uses SDL_SCANCODE_* via event.key.keysym.scancode.
 * In SDL 1.2 on Nspire, we use SDLK_* values.
 * We remap SDL_SCANCODE to SDLK equivalents. */
#define SDL_SCANCODE_UP        SDLK_UP
#define SDL_SCANCODE_DOWN      SDLK_DOWN
#define SDL_SCANCODE_LEFT      SDLK_LEFT
#define SDL_SCANCODE_RIGHT     SDLK_RIGHT
#define SDL_SCANCODE_ESCAPE    SDLK_ESCAPE
#define SDL_SCANCODE_SPACE     SDLK_SPACE
#define SDL_SCANCODE_RETURN    SDLK_RETURN
#define SDL_SCANCODE_P         SDLK_p
#define SDL_SCANCODE_E         SDLK_e
#define SDL_SCANCODE_O         SDLK_o
#define SDL_SCANCODE_K         SDLK_k
#define SDL_SCANCODE_Z         SDLK_z
#define SDL_SCANCODE_X         SDLK_x
#define SDL_SCANCODE_F1        SDLK_F1
#define SDL_SCANCODE_F2        SDLK_F2
#define SDL_SCANCODE_F3        SDLK_F3
#define SDL_SCANCODE_F4        SDLK_F4
#define SDL_SCANCODE_F5        SDLK_F5
#define SDL_SCANCODE_F6        SDLK_F6
#define SDL_SCANCODE_F7        SDLK_F7
#define SDL_SCANCODE_F8        SDLK_F8
#define SDL_SCANCODE_F9        SDLK_F9
/* Letters */
#define SDL_SCANCODE_A         SDLK_a
#define SDL_SCANCODE_B         SDLK_b
#define SDL_SCANCODE_C         SDLK_c
#define SDL_SCANCODE_D         SDLK_d
#define SDL_SCANCODE_F         SDLK_f
#define SDL_SCANCODE_G         SDLK_g
#define SDL_SCANCODE_H         SDLK_h
#define SDL_SCANCODE_I         SDLK_i
#define SDL_SCANCODE_J         SDLK_j
#define SDL_SCANCODE_L         SDLK_l
#define SDL_SCANCODE_M         SDLK_m
#define SDL_SCANCODE_N         SDLK_n
#define SDL_SCANCODE_Q         SDLK_q
#define SDL_SCANCODE_R         SDLK_r
#define SDL_SCANCODE_S         SDLK_s
#define SDL_SCANCODE_T         SDLK_t
#define SDL_SCANCODE_U         SDLK_u
#define SDL_SCANCODE_V         SDLK_v
#define SDL_SCANCODE_W         SDLK_w
#define SDL_SCANCODE_Y         SDLK_y

/* ---- Window/Renderer/Texture stubs ---- */
#define SDL_CreateWindow(t,x,y,w,h,f)    ((SDL_Window*)NULL)
#define SDL_DestroyWindow(w)              ((void)0)
#define SDL_CreateRenderer(w,i,f)         ((SDL_Renderer*)NULL)
#define SDL_DestroyRenderer(r)            ((void)0)
#define SDL_CreateTexture(r,f,a,w,h)      ((SDL_Texture*)NULL)
#define SDL_DestroyTexture(t)             ((void)0)
#define SDL_RenderCopy(r,t,s,d)           (0)
#define SDL_RenderPresent(r)              ((void)0)
#define SDL_RenderClear(r)                (0)
#define SDL_SetRenderDrawColor(r,R,G,B,A) (0)
#define SDL_RenderSetLogicalSize(r,w,h)   (0)
#define SDL_SetWindowSize(w,x,y)          ((void)0)
#define SDL_SetWindowFullscreen(w,f)      (0)
#define SDL_SetWindowIcon(w,icon)         ((void)0)
#define SDL_LockTexture(t,r,p,pitch)      (0)
#define SDL_UnlockTexture(t)              ((void)0)
#define SDL_UpdateTexture(t,r,p,pitch)    (0)
#define SDL_ShowCursor(toggle)            (0)
#define SDL_SetHint(name, value)          (0)

/* ---- Window flags ---- */
#define SDL_WINDOW_FULLSCREEN            0x00000001
#define SDL_WINDOW_FULLSCREEN_DESKTOP    0x00001001
#define SDL_WINDOWPOS_UNDEFINED          0

/* ---- Texture constants ---- */
#define SDL_TEXTUREACCESS_STREAMING      1
#define SDL_PIXELFORMAT_ARGB8888         0

/* ---- Hints ---- */
#define SDL_HINT_RENDER_SCALE_QUALITY    "SDL_RENDER_SCALE_QUALITY"

/* ---- Init flags ---- */
#ifndef SDL_INIT_EVENTS
#define SDL_INIT_EVENTS 0
#endif

/* ---- Audio stubs (sound disabled on Nspire) ---- */
typedef Uint16 SDL_AudioFormat;
typedef Uint32 SDL_AudioDeviceID;

#ifndef AUDIO_U8
#define AUDIO_U8     0x0008
#endif

#ifndef SDL_MIX_MAXVOLUME
#define SDL_MIX_MAXVOLUME 128
#endif

typedef struct SDL_AudioSpec_compat {
    int freq;
    SDL_AudioFormat format;
    Uint8 channels;
    Uint16 samples;
    void (*callback)(void *userdata, Uint8 *stream, int len);
    void *userdata;
} SDL_AudioSpec_compat;
#define SDL_AudioSpec SDL_AudioSpec_compat

#define SDL_OpenAudioDevice(dev, cap, des, obt, ac) (0)
#define SDL_CloseAudioDevice(dev)                   ((void)0)
#define SDL_PauseAudioDevice(dev, pause_on)         ((void)0)
#define SDL_PauseAudio(pause_on)                    ((void)0)
#define SDL_CloseAudio()                            ((void)0)
#define SDL_CreateMutex()                           ((SDL_mutex*)NULL)
#define SDL_DestroyMutex(m)                         ((void)0)
#define SDL_mutexP(m)                               (0)
#define SDL_mutexV(m)                               (0)
#define SDL_LoadWAV_RW(src, freesrc, spec, buf, len) (NULL)
#define SDL_FreeWAV(buf)                            ((void)0)

/* ---- Logging ---- */
#define SDL_Log(...) ((void)0)

/* ---- Memory ---- */
#ifndef SDL_free
#define SDL_free free
#endif
#ifndef SDL_malloc
#define SDL_malloc malloc
#endif

/* ---- Nspire-specific ---- */
#include <libndls.h>

#endif /* NSPIRE_SDL2_COMPAT_H */
