/*
 * xrick_nspire.c - Main entry point for TI-Nspire CX CAS II
 *
 * Replaces the original xrick.c.
 * Handles Nspire-specific initialization: chdir to .tns directory,
 * SDL init, and game launch.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libndls.h>
#include <SDL/SDL.h>

#include "system.h"
#include "sysarg.h"
#include "sysvid.h"
#include "game.h"
#include "fb.h"

/*
 * Get the directory containing the executable and chdir to it.
 * On Nspire, argv[0] is the full path to the .tns file.
 */
static void nspire_chdir_to_exe(const char *argv0)
{
    char dir[256];
    const char *last_slash;

    if (!argv0) return;

    last_slash = strrchr(argv0, '/');
    if (!last_slash) return;

    size_t len = last_slash - argv0;
    if (len >= sizeof(dir)) len = sizeof(dir) - 1;

    memcpy(dir, argv0, len);
    dir[len] = '\0';

    chdir(dir);
}

/*
 * Initialize system for Nspire
 */
void sys_init(int argc, char **argv)
{
    /* Change to executable directory so data files can be found */
    if (argc > 0)
        nspire_chdir_to_exe(argv[0]);

    /* Initialize SDL 1.2 (video only, no audio/timer - nSDL has no thread support) */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        exit(1);

    /* Initialize video subsystem */
    sysvid_init(FB_WIDTH, FB_HEIGHT);

    atexit(sys_shutdown);
}

/*
 * Shutdown system
 */
void sys_shutdown(void)
{
    sysvid_shutdown();
    SDL_Quit();
}

/*
 * main
 */
int main(int argc, char *argv[])
{
    /* Check for Nspire CX (color screen required) */
    if (!has_colors) {
        /* Monochrome Nspire not supported */
        return 1;
    }

    sys_init(argc, argv);

    /* xrick embeds all game data (maps, sprites, tiles) in the binary.
     * Sound files would be loaded from data path, but sound is disabled.
     * We pass "." as the data path (current directory). */
    game_run(".");

    sys_shutdown();
    return 0;
}
