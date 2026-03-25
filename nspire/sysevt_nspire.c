/*
 * sysevt_nspire.c - Event handling for TI-Nspire CX CAS II
 *
 * Replaces the original sysevt.c.
 * Uses Ndless isKeyPressed() for direct keyboard polling and
 * SDL 1.2 event loop for compatibility.
 */

#include <SDL/SDL.h>
#include <libndls.h>

#include "system.h"
#include "syskbd.h"
#include "sysvid.h"
#include "game.h"
#include "debug.h"
#include "control.h"
#include "draw.h"
#include "saveload.h"

#define SETBIT(x,b) x |= (b)
#define CLRBIT(x,b) x &= ~(b)

/*
 * Nspire key mapping using isKeyPressed():
 *
 * 8/2/4/6 -> UP/DOWN/LEFT/RIGHT
 * Enter/Return/Ctrl/Tab -> FIRE
 * Esc (Nspire: on/home) -> EXIT
 * P/Del -> PAUSE
 * E -> END (end game)
 *
 * Note: arrow keys removed - they share a keyboard matrix row
 * and ghost when pressed simultaneously (can't jump + move).
 */

/*
 * Process events using direct Nspire key polling
 */
void sysevt_poll(void)
{
    SDL_Event event;

    /* Process SDL events for compatibility */
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                SETBIT(control_status, CONTROL_EXIT);
                control_last = CONTROL_EXIT;
            }
            break;
        case SDL_QUIT:
            SETBIT(control_status, CONTROL_EXIT);
            control_last = CONTROL_EXIT;
            break;
        default:
            break;
        }
    }

    /* Direct Nspire key polling for responsive controls */

    /* Directions */
    if (isKeyPressed(KEY_NSPIRE_8)) {
        SETBIT(control_status, CONTROL_UP);
        control_last = CONTROL_UP;
    } else {
        CLRBIT(control_status, CONTROL_UP);
    }

    if (isKeyPressed(KEY_NSPIRE_2)) {
        SETBIT(control_status, CONTROL_DOWN);
        control_last = CONTROL_DOWN;
    } else {
        CLRBIT(control_status, CONTROL_DOWN);
    }

    if (isKeyPressed(KEY_NSPIRE_4)) {
        SETBIT(control_status, CONTROL_LEFT);
        control_last = CONTROL_LEFT;
    } else {
        CLRBIT(control_status, CONTROL_LEFT);
    }

    if (isKeyPressed(KEY_NSPIRE_6)) {
        SETBIT(control_status, CONTROL_RIGHT);
        control_last = CONTROL_RIGHT;
    } else {
        CLRBIT(control_status, CONTROL_RIGHT);
    }

    /* Fire: Enter or Ctrl (Tab = save, Del = load) */
    if (isKeyPressed(KEY_NSPIRE_ENTER) ||
        isKeyPressed(KEY_NSPIRE_RET) ||
        isKeyPressed(KEY_NSPIRE_CTRL)) {
        SETBIT(control_status, CONTROL_FIRE);
        control_last = CONTROL_FIRE;
    } else {
        CLRBIT(control_status, CONTROL_FIRE);
    }

    /* Pause: P */
    if (isKeyPressed(KEY_NSPIRE_P)) {
        SETBIT(control_status, CONTROL_PAUSE);
        control_last = CONTROL_PAUSE;
    } else {
        CLRBIT(control_status, CONTROL_PAUSE);
    }

    /* End game: E */
    if (isKeyPressed(KEY_NSPIRE_E)) {
        SETBIT(control_status, CONTROL_END);
        control_last = CONTROL_END;
    } else {
        CLRBIT(control_status, CONTROL_END);
    }

    /* Exit: Esc */
    if (isKeyPressed(KEY_NSPIRE_ESC)) {
        SETBIT(control_status, CONTROL_EXIT);
        control_last = CONTROL_EXIT;
    }

    /* Quick save: Tab */
    {
        static U8 tab_prev = 0;
        U8 tab_now = isKeyPressed(KEY_NSPIRE_TAB);
        if (tab_now && !tab_prev) {
            saveload_save();
        }
        tab_prev = tab_now;
    }

    /* Quick load: Del */
    {
        static U8 del_prev = 0;
        U8 del_now = isKeyPressed(KEY_NSPIRE_DEL);
        if (del_now && !del_prev) {
            saveload_load();
        }
        del_prev = del_now;
    }
}

/*
 * Wait for an event, then return.
 * On Nspire, we poll with a small delay to avoid busy-waiting.
 */
void sysevt_wait(void)
{
    while (1) {
        sysevt_poll();
        if (control_status)
            return;
        SDL_Delay(50);
    }
}
