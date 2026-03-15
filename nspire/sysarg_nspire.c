/*
 * sysarg_nspire.c - Command-line arguments stub for TI-Nspire
 *
 * Replaces the original sysarg.c which uses SDL2 scancodes.
 * On Nspire, there are no command-line arguments.
 */

#include "system.h"
#include "sysarg.h"
#include "game.h"

int sysarg_args_period = 0;
int sysarg_args_map = 0;
int sysarg_args_submap = 0;
int sysarg_args_fullscreen = 0;
int sysarg_args_zoom = 0;
char *sysarg_args_data = NULL;

void sysarg_init(int argc, char **argv)
{
    /* No command-line arguments on Nspire */
    (void)argc;
    (void)argv;
}
