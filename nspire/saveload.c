/*
 * saveload.c - Quick save/load system for TI-Nspire
 *
 * Saves/loads the complete game state (~1.2 KB) to xrick.sav.tns.
 * After loading, the map and entities are re-rendered from the restored state.
 */

#ifdef NSPIRE

#include <stdio.h>
#include <string.h>

#include "system.h"
#include "game.h"
#include "env.h"
#include "ents.h"
#include "maps.h"
#include "e_rick.h"
#include "e_bomb.h"
#include "e_bullet.h"
#include "e_sbonus.h"
#include "e_them.h"
#include "fb.h"
#include "draw.h"
#include "tiles.h"

/* Save file magic and version */
#define SAVE_MAGIC  0x58534156  /* "XSAV" */
#define SAVE_VERSION 1

/* e_rick static variables struct (defined in e_rick.c) */
typedef struct {
    U8 scrawl, trigger, ylow, seq, save_crawl;
    S8 offsx;
    S16 offsy;
    U16 save_x, save_y;
} e_rick_statics_t;

extern void e_rick_get_statics(e_rick_statics_t *out);
extern void e_rick_set_statics(const e_rick_statics_t *in);
extern void game_get_save_state(U8 *out_save_map_row, U8 *out_game_state_val);
extern void game_set_save_state(U8 in_save_map_row, U8 in_game_state_val);

/* Complete save state structure */
typedef struct {
    /* Header */
    U32 magic;
    U8 version;

    /* Environment */
    U8 env_lives;
    U8 env_bombs;
    U8 env_bullets;
    U32 env_score;
    U16 env_map;
    U16 env_submap;
    U8 env_changeSubmap;
    U8 env_trainer;
    U8 env_invicible;
    U8 env_highlight;

    /* Game state */
    U8 game_dir;
    U8 game_waitevt;
    U8 save_map_row;
    U8 game_state_val;

    /* Rick state */
    U8 e_rick_state;
    U8 e_rick_atExit;
    U16 e_rick_stop_x;
    U16 e_rick_stop_y;
    e_rick_statics_t rick_statics;

    /* Entity state */
    ent_t ent_ents[ENT_ENTSNUM + 1];

    /* Bomb/bullet/bonus state */
    U8 e_bomb_lethal;
    U8 e_bomb_ticker;
    S8 e_bullet_offsx;
    U8 e_sbonus_counting;
    U8 e_sbonus_counter;
    U16 e_sbonus_bonus;
    U32 e_them_rndseed;

    /* Map state */
    U8 map_frow;
    U8 map_tilesBank;
    U8 map_map[0x2C][0x20];
    U8 map_eflg[0x100];
} savestate_t;

#define SAVE_FILENAME "xrick.sav"

/*
 * Save game state to file
 */
void saveload_save(void)
{
    FILE *f;
    savestate_t state;

    memset(&state, 0, sizeof(state));

    /* Header */
    state.magic = SAVE_MAGIC;
    state.version = SAVE_VERSION;

    /* Environment */
    state.env_lives = env_lives;
    state.env_bombs = env_bombs;
    state.env_bullets = env_bullets;
    state.env_score = env_score;
    state.env_map = env_map;
    state.env_submap = env_submap;
    state.env_changeSubmap = env_changeSubmap;
    state.env_trainer = env_trainer;
    state.env_invicible = env_invicible;
    state.env_highlight = env_highlight;

    /* Game state */
    state.game_dir = game_dir;
    state.game_waitevt = game_waitevt;
    game_get_save_state(&state.save_map_row, &state.game_state_val);

    /* Rick state */
    state.e_rick_state = e_rick_state;
    state.e_rick_atExit = e_rick_atExit;
    state.e_rick_stop_x = e_rick_stop_x;
    state.e_rick_stop_y = e_rick_stop_y;
    e_rick_get_statics(&state.rick_statics);

    /* Entities */
    memcpy(state.ent_ents, ent_ents, sizeof(state.ent_ents));

    /* Bomb/bullet/bonus */
    state.e_bomb_lethal = e_bomb_lethal;
    state.e_bomb_ticker = e_bomb_ticker;
    state.e_bullet_offsx = e_bullet_offsx;
    state.e_sbonus_counting = e_sbonus_counting;
    state.e_sbonus_counter = e_sbonus_counter;
    state.e_sbonus_bonus = e_sbonus_bonus;
    state.e_them_rndseed = e_them_rndseed;

    /* Map */
    state.map_frow = map_frow;
    state.map_tilesBank = map_tilesBank;
    memcpy(state.map_map, map_map, sizeof(state.map_map));
    memcpy(state.map_eflg, map_eflg, sizeof(state.map_eflg));

    /* Write to file */
    f = fopen(SAVE_FILENAME, "wb");
    if (f) {
        fwrite(&state, sizeof(state), 1, f);
        fclose(f);
    }
}

/*
 * Load game state from file and restore
 */
void saveload_load(void)
{
    FILE *f;
    savestate_t state;

    f = fopen(SAVE_FILENAME, "rb");
    if (!f) return;

    if (fread(&state, sizeof(state), 1, f) != 1) {
        fclose(f);
        return;
    }
    fclose(f);

    /* Validate */
    if (state.magic != SAVE_MAGIC || state.version != SAVE_VERSION)
        return;

    /* Environment */
    env_lives = state.env_lives;
    env_bombs = state.env_bombs;
    env_bullets = state.env_bullets;
    env_score = state.env_score;
    env_map = state.env_map;
    env_submap = state.env_submap;
    env_changeSubmap = state.env_changeSubmap;
    env_trainer = state.env_trainer;
    env_invicible = state.env_invicible;
    env_highlight = state.env_highlight;

    /* Game state */
    game_dir = state.game_dir;
    game_waitevt = FALSE;  /* don't restore wait state */
    game_set_save_state(state.save_map_row, state.game_state_val);

    /* Rick state */
    e_rick_state = state.e_rick_state;
    e_rick_atExit = state.e_rick_atExit;
    e_rick_stop_x = state.e_rick_stop_x;
    e_rick_stop_y = state.e_rick_stop_y;
    e_rick_set_statics(&state.rick_statics);

    /* Entities */
    memcpy(ent_ents, state.ent_ents, sizeof(state.ent_ents));

    /* Bomb/bullet/bonus */
    e_bomb_lethal = state.e_bomb_lethal;
    e_bomb_ticker = state.e_bomb_ticker;
    e_bullet_offsx = state.e_bullet_offsx;
    e_sbonus_counting = state.e_sbonus_counting;
    e_sbonus_counter = state.e_sbonus_counter;
    e_sbonus_bonus = state.e_sbonus_bonus;
    e_them_rndseed = state.e_them_rndseed;

    /* Map */
    map_frow = state.map_frow;
    map_tilesBank = state.map_tilesBank;
    memcpy(map_map, state.map_map, sizeof(state.map_map));
    memcpy(map_eflg, state.map_eflg, sizeof(state.map_eflg));

    /* Re-render the screen from restored state */
    fb_clear();
    tiles_setBank(map_tilesBank);
    maps_paint();
    env_paintGame();
    env_paintXtra();
    game_rects = &draw_SCREENRECT;
}

#endif /* NSPIRE */
