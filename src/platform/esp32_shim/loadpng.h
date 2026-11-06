#ifndef _ESP32_SHIM_LOADPNG_H
#define _ESP32_SHIM_LOADPNG_H

#include <allegro.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void register_png_file_type(void) {}
BITMAP *load_png(const char *filename, RGB *pal);

#ifdef __cplusplus
}
#endif

#endif
