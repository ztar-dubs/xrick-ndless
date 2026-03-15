# xrick-ndless

Port of [xrick](https://github.com/zpqrtbnk/xrick) (open source clone of **Rick Dangerous**) for the **TI-Nspire CX CAS II** calculator, using the Ndless SDK.

All game data (sprites, tiles, maps, images) is embedded in the binary. No external data files are needed.

## Prerequisites

- [Ndless SDK](https://github.com/ndless-nspire/Ndless) installed and in your PATH:
  - `nspire-gcc` (ARM cross-compiler)
  - `genzehn`
  - `make-prg`
- `make` (GNU Make)

## Building

```bash
make
```

The build pipeline compiles all sources, links them into an ELF, then converts to the Nspire executable format:

```
.c -> .o -> xrick.elf -> xrick.zehn -> xrick.tns -> prod/xrick.tns
```

The final `xrick.tns` (~489 KB) is automatically copied to the `prod/` directory.

To clean build artifacts:

```bash
make clean
```

## Project structure

```
xrick-ndless/
|
|-- Makefile              Build system
|
|-- include/              Original xrick headers
|   |-- config.h          Build configuration (NSPIRE ifdefs for sound/log/zlib)
|   |-- system.h          Base types (U8, U16, U32, S8, S16, S32)
|   |-- fb.h              Framebuffer (320x200, 8-bit palettized)
|   |-- sysvid.h          Video layer interface
|   |-- sysevt.h          Event handling interface
|   |-- game.h            Game loop and state machine
|   |-- ents.h            Entity system (Rick, enemies, objects)
|   |-- maps.h            Map/submap management
|   |-- tiles.h           Tile rendering (8x8 blocks)
|   |-- sprites.h         Sprite rendering
|   |-- ...               Other headers (draw, rects, control, etc.)
|
|-- src/                  Original xrick sources (from zpqrtbnk/xrick)
|   |-- game.c            Main game loop and state machine
|   |-- data.c            Data file loading (path management)
|   |-- fb.c              Framebuffer operations, palette, fade in/out
|   |-- draw.c            Drawing primitives and screen rectangles
|   |-- maps.c            Map painting and scrolling
|   |-- tiles.c           Tile rendering
|   |-- sprites.c         Sprite rendering (with depth/clipping)
|   |-- ents.c            Entity management and dirty rect tracking
|   |-- scroller.c        Vertical scrolling engine
|   |-- e_rick.c          Rick entity logic (movement, actions)
|   |-- e_them.c          Enemy entity logic
|   |-- e_bomb.c          Bomb logic
|   |-- e_bullet.c        Bullet logic
|   |-- e_bonus.c         Bonus item logic
|   |-- e_box.c           Box/crate logic
|   |-- e_sbonus.c        Secret bonus logic
|   |-- env.c             Environment (score, lives, status bar)
|   |-- control.c         Control state (button flags)
|   |-- rects.c           Rectangle list (alloc/free)
|   |-- img.c             Image painting
|   |-- scr_xrick.c       Splash screen (embeds img_splash.e)
|   |-- scr_imain.c       Main intro screen
|   |-- scr_imap.c        Map intro screen
|   |-- scr_gameover.c    Game over screen
|   |-- scr_getname.c     High score name entry
|   |-- scr_pause.c       Pause screen
|   |-- sounds.c          Sound loading (disabled on Nspire)
|   |-- system.c          System utilities (time, sleep, panic)
|   |-- sysjoy.c          Joystick (disabled on Nspire)
|   |-- dat_*.c           Embedded game data (entities, maps, sprites, tiles)
|   |-- img_splash.e      Splash screen image data (included by scr_xrick.c)
|   |-- img_icon.e        Icon image data
|   |-- wav_*.e           Embedded WAV data (not used on Nspire)
|   |-- sdlcodes.e        SDL2 scancode table (not used on Nspire)
|   |                     --- Files below are NOT compiled (replaced by nspire/) ---
|   |-- xrick.c           Original main/init (SDL2, Windows)
|   |-- sysvid.c          Original video (SDL2 renderer/texture)
|   |-- sysevt.c          Original events (SDL2 scancodes)
|   |-- syskbd.c          Original keyboard mapping (SDL2)
|   |-- syssnd.c          Original sound (SDL2 audio)
|   |-- sysarg.c          Original command-line args (SDL2 scancodes)
|   |-- unzip.c           ZIP support (zlib, disabled)
|   |-- devtools.c        Dev tools (disabled)
|
|-- nspire/               Nspire platform layer
|   |-- SDL.h             SDL2-to-SDL1.2 compatibility header (nSDL)
|   |-- config.h          (unused, config is in include/config.h with #ifdef NSPIRE)
|   |-- xrick_nspire.c    Entry point: chdir to .tns dir, SDL init, launch game
|   |-- sysvid_nspire.c   Video: SDL1.2 surface 320x240 RGB565, palette lookup,
|   |                      optimized scanline conversion (4px unroll, U32 writes),
|   |                      dirty rect merging, SDL_UpdateRects
|   |-- sysevt_nspire.c   Events: direct key polling via isKeyPressed() (ndless)
|   |-- syskbd_nspire.c   Keyboard stubs (controls handled in sysevt)
|   |-- syssnd_nspire.c   Sound stubs (audio disabled)
|   |-- sysarg_nspire.c   Command-line args stubs (no args on Nspire)
|   |-- nspire_fopen.c    File I/O wrappers: auto .tns suffix via linker --wrap
|   |                      (compiled without -flto for --wrap visibility)
|
|-- prod/                 Distribution files
|   |-- xrick.tns         Ready-to-use Nspire binary
|   |-- doc/
|       |-- readme.html   Documentation (English)
|       |-- lisezmoi.html Documentation (French)
```

## Architecture

The game engine (`src/`) is the original xrick code with minimal modifications:

- `include/config.h`: `#ifdef NSPIRE` blocks to disable sound, logging, debug, and zlib
- `src/data.c`: `_strdup` replaced by `strdup` (GCC compatibility)
- `src/img.c`: removed duplicate `IMG_SPLASH` definition (LTO conflict)
- `src/game.c`: added `rects_free(ent_rects)` at exit for proper cleanup

The Nspire platform layer (`nspire/`) replaces 6 original source files with Nspire-specific implementations. The `nspire/` directory has higher include priority (`-Inspire`) so that `nspire/SDL.h` intercepts `#include <SDL.h>` and provides SDL2-to-SDL1.2 compatibility.

### Rendering pipeline

```
Game logic (game.c)
  |-> Entities paint into 8-bit framebuffer (320x200, indexed colors)
  |-> Dirty rectangles tracked per entity
  |
  v
sysvid_update (sysvid_nspire.c)
  |-> Merge overlapping dirty rects
  |-> Convert 8-bit indexed -> RGB565 via palette lookup (unrolled x4)
  |-> SDL_UpdateRects (partial LCD update, not full flip)
```

## Controls

| Key | Action |
|-----|--------|
| **8** | Up / Jump |
| **2** | Down / Crouch |
| **4** | Left |
| **6** | Right |
| **Enter** / **Ctrl** / **Tab** | Fire |
| **P** / **Del** | Pause |
| **E** | End game |
| **Esc** | Quit |

Numpad keys are used instead of arrow keys to avoid keyboard matrix ghosting (arrows share a matrix row and can't be pressed simultaneously).

## Credits

- **xrick** by bigorno - [https://github.com/zpqrtbnk/xrick](https://github.com/zpqrtbnk/xrick) (version 050500, SDL2 branch)
- **Rick Dangerous** (1989) by Core Design, published by MicroProse / Firebird Software
- **Ndless SDK** - [https://github.com/ndless-nspire/Ndless](https://github.com/ndless-nspire/Ndless)

## License

Released in the spirit of the GNU GPL (following the original xrick license).
