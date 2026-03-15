/*
 * syskbd_nspire.c - Keyboard definitions for TI-Nspire
 *
 * Replaces the original syskbd.c.
 * On Nspire, we use direct key polling (isKeyPressed) in sysevt_nspire.c,
 * so these values are only used for sysarg key remapping compatibility.
 * We set them to SDLK values that match our SDL.h compatibility defines.
 */

#include "system.h"

/* These are referenced by sysarg.c for the -keys option.
 * On Nspire we don't use command-line args, but they must exist. */
U8 syskbd_up = 0;     /* handled via isKeyPressed */
U8 syskbd_down = 0;
U8 syskbd_left = 0;
U8 syskbd_right = 0;
U8 syskbd_pause = 0;
U8 syskbd_end = 0;
U8 syskbd_xtra = 0;
U8 syskbd_fire = 0;
