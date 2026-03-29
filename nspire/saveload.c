/*
 * saveload.c - Quick save/load system for TI-Nspire
 *
 * Saves/loads the complete game state to xrick.sav.tns.
 * Uses sequential binary read/write to avoid struct padding issues on ARM.
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
#define SAVE_VERSION 2

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

#define SAVE_FILENAME "xrick.sav"

/* Helper: write raw bytes to file */
static void wr(FILE *f, const void *p, size_t n) { fwrite(p, 1, n, f); }
/* Helper: read raw bytes from file */
static int rd(FILE *f, void *p, size_t n) { return fread(p, 1, n, f) == n; }

/*
 * Save game state to file
 */
void saveload_save(void)
{
    FILE *f;
    U32 magic = SAVE_MAGIC;
    U8 ver = SAVE_VERSION;
    U8 smr, gsv;
    e_rick_statics_t rs;

    f = fopen(SAVE_FILENAME, "wb");
    if (!f) return;

    /* Header */
    wr(f, &magic, 4);
    wr(f, &ver, 1);

    /* Environment */
    wr(f, &env_lives, 1);
    wr(f, &env_bombs, 1);
    wr(f, &env_bullets, 1);
    wr(f, &env_score, 4);
    wr(f, &env_map, 2);
    wr(f, &env_submap, 2);
    wr(f, &env_changeSubmap, 1);
    wr(f, &env_trainer, 1);
    wr(f, &env_invicible, 1);
    wr(f, &env_highlight, 1);

    /* Game state */
    wr(f, &game_dir, 1);
    game_get_save_state(&smr, &gsv);
    wr(f, &smr, 1);

    /* Rick state */
    wr(f, &e_rick_state, 1);
    wr(f, &e_rick_atExit, 1);
    wr(f, &e_rick_stop_x, 2);
    wr(f, &e_rick_stop_y, 2);
    e_rick_get_statics(&rs);
    wr(f, &rs.scrawl, 1);
    wr(f, &rs.trigger, 1);
    wr(f, &rs.ylow, 1);
    wr(f, &rs.seq, 1);
    wr(f, &rs.save_crawl, 1);
    wr(f, &rs.offsx, 1);
    wr(f, &rs.offsy, 2);
    wr(f, &rs.save_x, 2);
    wr(f, &rs.save_y, 2);

    /* Entities - write each field individually to avoid padding */
    {
        int i;
        for (i = 0; i <= ENT_ENTSNUM; i++) {
            wr(f, &ent_ents[i].n, 1);
            wr(f, &ent_ents[i].x, 2);
            wr(f, &ent_ents[i].y, 2);
            wr(f, &ent_ents[i].sprite, 1);
            wr(f, &ent_ents[i].w, 1);
            wr(f, &ent_ents[i].h, 1);
            wr(f, &ent_ents[i].mark, 2);
            wr(f, &ent_ents[i].flags, 1);
            wr(f, &ent_ents[i].trig_x, 2);
            wr(f, &ent_ents[i].trig_y, 2);
            wr(f, &ent_ents[i].xsave, 2);
            wr(f, &ent_ents[i].ysave, 2);
            wr(f, &ent_ents[i].sprbase, 2);
            wr(f, &ent_ents[i].step_no_i, 2);
            wr(f, &ent_ents[i].step_no, 2);
            wr(f, &ent_ents[i].c1, 2);
            wr(f, &ent_ents[i].c2, 2);
            wr(f, &ent_ents[i].ylow, 1);
            wr(f, &ent_ents[i].offsy, 2);
            wr(f, &ent_ents[i].latency, 1);
            wr(f, &ent_ents[i].prev_n, 1);
            wr(f, &ent_ents[i].prev_x, 2);
            wr(f, &ent_ents[i].prev_y, 2);
            wr(f, &ent_ents[i].prev_s, 1);
            wr(f, &ent_ents[i].front, 1);
            wr(f, &ent_ents[i].trigsnd, 1);
        }
    }

    /* Bomb/bullet/bonus */
    wr(f, &e_bomb_lethal, 1);
    wr(f, &e_bomb_ticker, 1);
    wr(f, &e_bullet_offsx, 1);
    wr(f, &e_sbonus_counting, 1);
    wr(f, &e_sbonus_counter, 1);
    wr(f, &e_sbonus_bonus, 2);
    wr(f, &e_them_rndseed, 4);

    /* Map */
    wr(f, &map_frow, 1);
    wr(f, &map_tilesBank, 1);
    wr(f, map_map, sizeof(map_map));
    wr(f, map_eflg, sizeof(map_eflg));

    fclose(f);
}

/*
 * Load game state from file and restore
 */
void saveload_load(void)
{
    FILE *f;
    U32 magic;
    U8 ver, smr;
    e_rick_statics_t rs;
    int ok = 1;

    f = fopen(SAVE_FILENAME, "rb");
    if (!f) return;

    /* Header */
    ok = ok && rd(f, &magic, 4);
    ok = ok && rd(f, &ver, 1);
    if (!ok || magic != SAVE_MAGIC || ver != SAVE_VERSION) {
        fclose(f);
        return;
    }

    /* Environment */
    ok = ok && rd(f, &env_lives, 1);
    ok = ok && rd(f, &env_bombs, 1);
    ok = ok && rd(f, &env_bullets, 1);
    ok = ok && rd(f, &env_score, 4);
    ok = ok && rd(f, &env_map, 2);
    ok = ok && rd(f, &env_submap, 2);
    ok = ok && rd(f, &env_changeSubmap, 1);
    ok = ok && rd(f, &env_trainer, 1);
    ok = ok && rd(f, &env_invicible, 1);
    ok = ok && rd(f, &env_highlight, 1);

    /* Game state */
    ok = ok && rd(f, &game_dir, 1);
    ok = ok && rd(f, &smr, 1);

    /* Rick state */
    ok = ok && rd(f, &e_rick_state, 1);
    ok = ok && rd(f, &e_rick_atExit, 1);
    ok = ok && rd(f, &e_rick_stop_x, 2);
    ok = ok && rd(f, &e_rick_stop_y, 2);
    ok = ok && rd(f, &rs.scrawl, 1);
    ok = ok && rd(f, &rs.trigger, 1);
    ok = ok && rd(f, &rs.ylow, 1);
    ok = ok && rd(f, &rs.seq, 1);
    ok = ok && rd(f, &rs.save_crawl, 1);
    ok = ok && rd(f, &rs.offsx, 1);
    ok = ok && rd(f, &rs.offsy, 2);
    ok = ok && rd(f, &rs.save_x, 2);
    ok = ok && rd(f, &rs.save_y, 2);

    if (ok) {
        e_rick_set_statics(&rs);
        /* Force CTRL_ACTION state - don't restore game_state to avoid
         * resuming in the middle of a transition (scroll, fade, etc.) */
        game_set_save_state(smr, 14); /* 14 = CTRL_ACTION */
        game_waitevt = FALSE;
    }

    /* Entities */
    {
        int i;
        for (i = 0; i <= ENT_ENTSNUM && ok; i++) {
            ok = ok && rd(f, &ent_ents[i].n, 1);
            ok = ok && rd(f, &ent_ents[i].x, 2);
            ok = ok && rd(f, &ent_ents[i].y, 2);
            ok = ok && rd(f, &ent_ents[i].sprite, 1);
            ok = ok && rd(f, &ent_ents[i].w, 1);
            ok = ok && rd(f, &ent_ents[i].h, 1);
            ok = ok && rd(f, &ent_ents[i].mark, 2);
            ok = ok && rd(f, &ent_ents[i].flags, 1);
            ok = ok && rd(f, &ent_ents[i].trig_x, 2);
            ok = ok && rd(f, &ent_ents[i].trig_y, 2);
            ok = ok && rd(f, &ent_ents[i].xsave, 2);
            ok = ok && rd(f, &ent_ents[i].ysave, 2);
            ok = ok && rd(f, &ent_ents[i].sprbase, 2);
            ok = ok && rd(f, &ent_ents[i].step_no_i, 2);
            ok = ok && rd(f, &ent_ents[i].step_no, 2);
            ok = ok && rd(f, &ent_ents[i].c1, 2);
            ok = ok && rd(f, &ent_ents[i].c2, 2);
            ok = ok && rd(f, &ent_ents[i].ylow, 1);
            ok = ok && rd(f, &ent_ents[i].offsy, 2);
            ok = ok && rd(f, &ent_ents[i].latency, 1);
            ok = ok && rd(f, &ent_ents[i].prev_n, 1);
            ok = ok && rd(f, &ent_ents[i].prev_x, 2);
            ok = ok && rd(f, &ent_ents[i].prev_y, 2);
            ok = ok && rd(f, &ent_ents[i].prev_s, 1);
            ok = ok && rd(f, &ent_ents[i].front, 1);
            ok = ok && rd(f, &ent_ents[i].trigsnd, 1);
        }
    }

    /* Bomb/bullet/bonus */
    ok = ok && rd(f, &e_bomb_lethal, 1);
    ok = ok && rd(f, &e_bomb_ticker, 1);
    ok = ok && rd(f, &e_bullet_offsx, 1);
    ok = ok && rd(f, &e_sbonus_counting, 1);
    ok = ok && rd(f, &e_sbonus_counter, 1);
    ok = ok && rd(f, &e_sbonus_bonus, 2);
    ok = ok && rd(f, &e_them_rndseed, 4);

    /* Map */
    ok = ok && rd(f, &map_frow, 1);
    ok = ok && rd(f, &map_tilesBank, 1);
    ok = ok && rd(f, map_map, sizeof(map_map));
    ok = ok && rd(f, map_eflg, sizeof(map_eflg));

    fclose(f);

    if (!ok) return;

    /* Re-render the screen from restored state */
    fb_clear();
    tiles_setBank(map_tilesBank);
    maps_paint();
    env_paintGame();
    env_paintXtra();
    game_rects = &draw_SCREENRECT;
}

#endif /* NSPIRE */
