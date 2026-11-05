-------------------------------------------

Fire Starter

August 15th, 2005
(FLCL Handheld Edition Update)

-------------------------------------------

For Allegro Speedhack 2005

by Marcello Bastea-Forte (marcello@cellosoft.com)

-------------------------------------------

Run firestarter.exe

controls:
	arrow keys to move
	enter (or 'a') to start game / advance intro narrative
	enter to pause / view city map during gameplay
	right shift to change music
	'a' to burn
	f2 to return to title screen
	
keys:
	escape quits
	f9 toggles fullscreen mode (non-functional)
	
-------------------------------------------

FLCL Handheld Edition Changes:

1. Title Screen Replacement (src/IntroSequence.h, src/IntroSequence.cpp):
   - Replaced the original splash title screen with a retro monochrome green
     "FIRE STARTER" title screen rendered over an isometric city backdrop
     built from the original game's building and road artwork.
   - Displays "press start" below the title and starts the introductory
     narrative sequence when Enter (START) or 'A' is pressed.

2. Six-Screen Introductory Narrative Sequence:
   - Implements a deterministic state machine (TITLE -> INTRO_FADE_IN ->
     INTRO_READING -> INTRO_FADE_OUT -> GAME_INITIALIZATION -> GAMEPLAY)
     presenting the six canonical FLCL Fire Starter narrative screens:
     [Screen 1]
     "No one knows how Endsville, the city of devils, was created, but as soon as it appeared, it started to grow, and slowly eat up the world."
     [Screen 2]
     "You are an agent of Canti, the God of the Black Flame... and you must stop Endsville from destroying the world."
     [Screen 3]
     "Your only weapon is flame:  matches, lighters, and fire-bombs. Get the weapons, outsmart the firefighters, hide from the police."
     [Screen 4]
     "Torch the city, and purge its demons. But you must be careful; you cannot burn everything."
     [Screen 5]
     "If the entire city burns down, there will be no place to live. Thus, the battle is endless: there is no final victory. The only thing you can do is burn... and burn... and burn..."
     [Screen 6]
     "Your highest goal is to receive the personal blessing of Canti, the God of the Black Flame..."
   - Includes 800 ms monochrome green fade-in and fade-out transitions,
     automatic reading timers (8.0-12.0s scaled to passage length), safe
     manual advancement via START (Enter) or A, and a smooth 800 ms fade
     directly into initialized gameplay after Screen 6.

3. Configurable Portrait Display Resolution (src/DisplayConfig.h):
   - Target display buffer defaults to 240 x 320 pixels (scaled 2x to 480 x 640
     on desktop via FIRESTARTER_WINDOW_SCALE).
   - MainGame preserves its internal 200 x 300 coordinate space and scales
     cleanly to the configured target display resolution without clipping.

-------------------------------------------

This game is based on a fictious game from the Gainax DVD movie, FLCL.

Quote from the actual movie:

	Fire Starter
	No one knows how the city of devils, Endsville, was born.
	As soon as it was created, it prospered,
	And then it slowly ate up the world.
	You serve Cantido, god of the black flame,
	And must hinder its growth by setting it afire.
	Trick the fire fighters, hide from the police,
	And torch the city to purge it of its devils.
	But you mustn't burn everying.
	If the entire city burns down, you will have no place to stay.
	You will not be able to live.
	There can be no final victory.
	The only thing you can do is burn the city.
	Your only hope is to receive the blessing of Cantido, god of black flames.

-------------------------------------------
Credits:

Lead, Coding:
	Marcello Bastea-Forte

Music, Graphics, and Sound effects:
	Zack Fowler
	
Additional Music:
	Travis Skare
	
Much code used from X-G's partypack

Additional Artwork:
	Maiko
	
-------------------------------------------

Compile with Makefile

Made with mingw-gcc on Windows XP.

Libraries used: libpng, loadpng, DUMB, Allegro 4

-------------------------------------------

COPYRIGHT 2005 CELLOSOFT AND MARCELLO BASTEA-FORTE

www.cellosoft.com

Source released under creative commons license:

http://creativecommons.org/licenses/by-nc-sa/2.0/

Graphics and Music cannot be used without permission.

-------------------------------------------
