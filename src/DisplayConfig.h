#ifndef _DISPLAY_CONFIG_H
#define _DISPLAY_CONFIG_H

// Target handheld display resolution (portrait TFT, default 240x320)
#ifndef FIRESTARTER_DISPLAY_WIDTH
#define FIRESTARTER_DISPLAY_WIDTH 240
#endif

#ifndef FIRESTARTER_DISPLAY_HEIGHT
#define FIRESTARTER_DISPLAY_HEIGHT 320
#endif

// Desktop window integer scale factor (1x native 240x320 on ESP32-S3, 2x = 480x640 on desktop)
#ifndef FIRESTARTER_WINDOW_SCALE
#ifdef FIRESTARTER_PLATFORM_ESP32
#define FIRESTARTER_WINDOW_SCALE 1
#else
#define FIRESTARTER_WINDOW_SCALE 2
#endif
#endif

// Original game internal logical dimensions (preserved for gameplay/HUD fidelity)
#define FIRESTARTER_GAME_NATIVE_WIDTH 200
#define FIRESTARTER_GAME_NATIVE_HEIGHT 300

#endif
