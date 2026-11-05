# FLCL – Fire Starter (Faithful Game Recreation & Handheld Port Foundation)

Based on the 2005 recreation by **Marcello Bastea-Forte** (`marcello3d/firestarter`), with original music, graphics, and sound effects by **Zack Fowler**, additional music by **Travis Skare**, and `partypack` utilities by **X-G**.

## Overview of Modifications

This branch (`flcl-handheld-port`) preserves the original 2005 gameplay baseline in full while introducing targeted enhancements required for a faithful recreation and future dedicated ESP32-S3 portrait handheld deployment:

1. **`FIRE STARTER` Title Screen ([`src/IntroSequence.h`](src/IntroSequence.h), [`src/IntroSequence.cpp`](src/IntroSequence.cpp))**:
   - Replaces the old character splash image (`data/title.png` is preserved untouched in `data/`) with a monochrome green `FIRE STARTER` title screen over a deterministic isometric city backdrop constructed from the original game's road and building tiles (`data/roads.png`, `data/building0.png`–`data/building3.png`, `data/construction0.png`, `data/rubble.png`).
   - Displays an unobtrusive `press start` instruction beneath the title and accepts `START` (`Enter`) or `A` (`A` key) to begin a new game.

2. **Six-Screen Introductory Narrative Sequence ([`src/IntroSequence.cpp`](src/IntroSequence.cpp))**:
   - Implements a deterministic state machine (`TITLE` -> `INTRO_FADE_IN` -> `INTRO_READING` -> `INTRO_FADE_OUT` -> `GAME_INITIALIZATION` -> `GAMEPLAY`) that presents all six narrative passages verbatim in order:
     1. `"No one knows how Endsville, the city of devils, was created, but as soon as it appeared, it started to grow, and slowly eat up the world."`
     2. `"You are an agent of Canti, the God of the Black Flame... and you must stop Endsville from destroying the world."`
     3. `"Your only weapon is flame:  matches, lighters, and fire-bombs. Get the weapons, outsmart the firefighters, hide from the police."` *(preserving the two spaces after `flame:`)*
     4. `"Torch the city, and purge its demons. But you must be careful; you cannot burn everything."`
     5. `"If the entire city burns down, there will be no place to live. Thus, the battle is endless: there is no final victory. The only thing you can do is burn... and burn... and burn..."`
     6. `"Your highest goal is to receive the personal blessing of Canti, the God of the Black Flame..."`
   - Smooth 800 ms monochrome green fade-in and fade-out transitions (`FADE_IN_TICKS = 80`, `FADE_OUT_TICKS = 80` at 100 Hz).
   - Automatic reading timers scaled to passage length (8.0s to 12.0s) and safe manual advancement via `START` (`Enter`) or `A`, ignoring inputs during active fade transitions.
   - Transitions directly from Screen 6 into `MainGame::setup()` with an 800 ms fade-in to gameplay.

3. **Configurable Portrait Display Resolution ([`src/DisplayConfig.h`](src/DisplayConfig.h), [`src/FireStarter.cpp`](src/FireStarter.cpp), [`src/MainGame.cpp`](src/MainGame.cpp))**:
   - Default target display resolution is `240 × 320` (`FIRESTARTER_DISPLAY_WIDTH`, `FIRESTARTER_DISPLAY_HEIGHT`), scaled by `FIRESTARTER_WINDOW_SCALE` (`2x` -> `480 × 640` desktop window).
   - `MainGame` renders to its internal `200 × 300` logical canvas (`gameCanvas`) and scales cleanly to the target display resolution so gameplay mechanics, collision math, and HUD elements remain 100% unchanged and unclipped.

## Controls

| Action | Key | Handheld Button |
| :--- | :--- | :--- |
| Move Player | Arrow Keys | D-Pad (`UP`, `DOWN`, `LEFT`, `RIGHT`) |
| Start Game / Advance Intro | `Enter` or `A` | `START` or `A` |
| Burn Building | `A` | `A` |
| Pause / Toggle Radar Map | `Enter` | `START` |
| Switch Music Track | `Right Shift` | `SELECT` |
| Return to Title Screen | `F2` | — |
| Quit (Desktop) | `Esc` | — |

## Building

```powershell
make clean
make all EXTRA_CFLAGS="-I../toolchain/mingw32/include" LIBDIR="-L../toolchain/mingw32/lib"
```

To override target display resolution or desktop window scale at compile time:

```powershell
make all EXTRA_CFLAGS="-I../toolchain/mingw32/include -DFIRESTARTER_DISPLAY_WIDTH=240 -DFIRESTARTER_DISPLAY_HEIGHT=320 -DFIRESTARTER_WINDOW_SCALE=2" LIBDIR="-L../toolchain/mingw32/lib"
```
