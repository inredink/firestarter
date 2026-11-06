#ifndef _ESP32_SHIM_ALDUMB_H
#define _ESP32_SHIM_ALDUMB_H

#include <allegro.h>
#include "../dumb/include/dumb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AL_DUH_PLAYER AL_DUH_PLAYER;

AL_DUH_PLAYER *al_start_duh(DUH *duh, int n_channels, long pos, float volume, long bufsize, int freq);
void al_stop_duh(AL_DUH_PLAYER *dp);
int al_poll_duh(AL_DUH_PLAYER *dp);

#ifdef __cplusplus
}
#endif

#endif
