/*
 * saveload.h - Quick save/load system for TI-Nspire
 */

#ifndef _SAVELOAD_H
#define _SAVELOAD_H

#ifdef NSPIRE

/* Save current game state to xrick.sav.tns */
extern void saveload_save(void);

/* Load game state from xrick.sav.tns and restore */
extern void saveload_load(void);

#endif /* NSPIRE */

#endif /* _SAVELOAD_H */
