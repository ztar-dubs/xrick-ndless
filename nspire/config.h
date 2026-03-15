/*
 * config.h - Nspire-specific configuration
 *
 * Overrides the original include/config.h.
 * Placed in nspire/ directory which has higher include priority.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/* version */
#define VERSION "050500-nspire"

/* graphics (choose one) */
#define GFXST
#undef GFXPC

/* logging disabled on Nspire (no console) */
#undef ENABLE_LOG

/* joystick support - disabled */
#undef ENABLE_JOYSTICK

/* sound support - disabled on Nspire (no audio hardware accessible) */
#undef ENABLE_SOUND

/* cheats support */
#define ENABLE_CHEATS

/* auto-defocus support - disabled */
#undef ENABLE_FOCUS

/* development tools - disabled */
#undef ENABLE_DEVTOOLS
#undef DEBUG

/* zlib - disabled, no external data files needed */
#define NOZLIB
#undef WITH_ZLIB

/* Nspire platform flag */
#define NSPIRE

#endif

/* eof */
