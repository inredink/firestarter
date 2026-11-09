#include "esp32_shim/allegro.h"
#include "esp32_shim/loadpng.h"
#include "esp32_shim/aldumb.h"
#include "EmbeddedAssetsData.h"

#include <math.h>

// ============================================================================
// Global Allegro State
// ============================================================================
BITMAP *screen = NULL;
volatile char key[KEY_MAX] = {0};
JOYSTICK_INFO joy[4] = {};

static int g_color_depth = 16;

struct TimerSlot {
    void (*proc)(void);
    int interval_ms;
    int accum_ms;
};

static TimerSlot g_timers[8];
static int g_timer_count = 0;

// Embedded screen flush hook (implemented by ESP32Main.cpp on hardware)
extern "C" void __attribute__((weak)) esp32_flush_screen_from_allegro(const uint16_t *pixels, int w, int h) {
    (void)pixels;
    (void)w;
    (void)h;
}

// ============================================================================
// Core System & Timers
// ============================================================================
extern "C" int allegro_init(void) {
    memset((void *)key, 0, sizeof(key));
    g_timer_count = 0;
    return 0;
}

extern "C" void allegro_exit(void) {
    if (screen) {
        destroy_bitmap(screen);
        screen = NULL;
    }
}

extern "C" void set_close_button_callback(void (*proc)(void)) {
    (void)proc;
}

extern "C" void set_window_title(const char *name) {
    (void)name;
}

extern "C" int install_keyboard(void) {
    return 0;
}

extern "C" int install_mouse(void) {
    return 0;
}

extern "C" void __attribute__((weak)) esp32_poll_buttons(void) {}

extern "C" int poll_keyboard(void) {
    esp32_poll_buttons();
    return 0;
}

extern "C" int poll_mouse(void) {
    return 0;
}

extern "C" int install_int(void (*proc)(void), int speed) {
    if (!proc) return -1;
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].proc == proc) {
            g_timers[i].interval_ms = (speed > 0) ? speed : 10;
            g_timers[i].accum_ms = 0;
            return 0;
        }
    }
    if (g_timer_count < 8) {
        g_timers[g_timer_count].proc = proc;
        g_timers[g_timer_count].interval_ms = (speed > 0) ? speed : 10;
        g_timers[g_timer_count].accum_ms = 0;
        g_timer_count++;
    }
    return 0;
}

extern "C" int install_int_ex(void (*proc)(void), int speed) {
    return install_int(proc, speed);
}

extern "C" void remove_int(void (*proc)(void)) {
    for (int i = 0; i < g_timer_count; i++) {
        if (g_timers[i].proc == proc) {
            for (int j = i; j < g_timer_count - 1; j++) {
                g_timers[j] = g_timers[j + 1];
            }
            g_timer_count--;
            return;
        }
    }
}

extern "C" void esp32_tick_timers(int elapsed_ms) {
    for (int i = 0; i < g_timer_count; i++) {
        g_timers[i].accum_ms += elapsed_ms;
        while (g_timers[i].accum_ms >= g_timers[i].interval_ms) {
            g_timers[i].accum_ms -= g_timers[i].interval_ms;
            if (g_timers[i].proc) {
                g_timers[i].proc();
            }
        }
    }
}

extern "C" void set_color_depth(int depth) {
    g_color_depth = depth;
}

extern "C" int set_gfx_mode(int card, int w, int h, int v_w, int v_h) {
    (void)card;
    (void)v_w;
    (void)v_h;
    if (screen) {
        destroy_bitmap(screen);
    }
    screen = create_bitmap(w, h);
    return screen ? 0 : -1;
}

extern "C" int set_display_switch_mode(int mode) {
    (void)mode;
    return 0;
}

extern "C" int set_display_switch_callback(int dir, void (*cb)(void)) {
    (void)dir;
    (void)cb;
    return 0;
}

// ============================================================================
// Color Management (16-bit RGB565, Magic Pink Mask = 0xF81F)
// ============================================================================
extern "C" int makecol(int r, int g, int b) {
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;

    if (r >= 248 && g <= 4 && b >= 248) {
        return 0xF81F;
    }
    uint16_t c = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    if (c == 0xF81F) {
        c = 0xF81E;
    }
    return (int)c;
}

extern "C" int getr(int c) {
    int r5 = (c >> 11) & 0x1F;
    return (r5 << 3) | (r5 >> 2);
}

extern "C" int getg(int c) {
    int g6 = (c >> 5) & 0x3F;
    return (g6 << 2) | (g6 >> 4);
}

extern "C" int getb(int c) {
    int b5 = c & 0x1F;
    return (b5 << 3) | (b5 >> 2);
}

extern "C" int bitmap_mask_color(BITMAP *bmp) {
    (void)bmp;
    return 0xF81F;
}

extern "C" int bitmap_color_depth(BITMAP *bmp) {
    (void)bmp;
    return 16;
}

extern "C" void select_palette(const RGB *p) {
    (void)p;
}

// ============================================================================
// Bitmap Management
// ============================================================================
extern "C" BITMAP *create_bitmap(int width, int height) {
    if (width <= 0 || height <= 0) return NULL;
    BITMAP *bmp = (BITMAP *)malloc(sizeof(BITMAP));
    if (!bmp) return NULL;
    bmp->pixels = (uint16_t *)calloc((size_t)width * (size_t)height, sizeof(uint16_t));
    if (!bmp->pixels) {
        free(bmp);
        return NULL;
    }
    bmp->w = width;
    bmp->h = height;
    bmp->stride = width;
    bmp->owns_memory = 1;
    bmp->cl = 0;
    bmp->ct = 0;
    bmp->cr = width;
    bmp->cb = height;
    bmp->clip = 1;
    return bmp;
}

extern "C" BITMAP *create_bitmap_ex(int color_depth, int width, int height) {
    // Handle both (depth, w, h) and accidental (w, h, depth) from alBitmap.h
    if ((height == 8 || height == 15 || height == 16 || height == 24 || height == 32) && color_depth > 32) {
        return create_bitmap(color_depth, width);
    }
    return create_bitmap(width, height);
}

extern "C" BITMAP *create_video_bitmap(int width, int height) {
    return create_bitmap(width, height);
}

extern "C" BITMAP *create_system_bitmap(int width, int height) {
    return create_bitmap(width, height);
}

extern "C" BITMAP *create_sub_bitmap(BITMAP *parent, int x, int y, int width, int height) {
    if (!parent) return NULL;
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > parent->w) width = parent->w - x;
    if (y + height > parent->h) height = parent->h - y;
    if (width <= 0 || height <= 0) return NULL;

    BITMAP *bmp = (BITMAP *)malloc(sizeof(BITMAP));
    if (!bmp) return NULL;
    bmp->w = width;
    bmp->h = height;
    bmp->stride = parent->stride;
    bmp->pixels = parent->pixels + y * parent->stride + x;
    bmp->owns_memory = 0;
    bmp->cl = 0;
    bmp->ct = 0;
    bmp->cr = width;
    bmp->cb = height;
    bmp->clip = 1;
    return bmp;
}

extern "C" void destroy_bitmap(BITMAP *bitmap) {
    if (!bitmap) return;
    if (bitmap->owns_memory && bitmap->pixels) {
        free(bitmap->pixels);
    }
    free(bitmap);
}

extern "C" int is_same_bitmap(BITMAP *bmp1, BITMAP *bmp2) {
    if (!bmp1 || !bmp2) return 0;
    return (bmp1 == bmp2 || bmp1->pixels == bmp2->pixels) ? 1 : 0;
}

extern "C" int is_linear_bitmap(BITMAP *bmp) { (void)bmp; return 1; }
extern "C" int is_memory_bitmap(BITMAP *bmp) { return (bmp && bmp != screen) ? 1 : 0; }
extern "C" int is_planar_bitmap(BITMAP *bmp) { (void)bmp; return 0; }
extern "C" int is_screen_bitmap(BITMAP *bmp) { return (bmp && bmp == screen) ? 1 : 0; }
extern "C" int is_sub_bitmap(BITMAP *bmp)    { return (bmp && !bmp->owns_memory) ? 1 : 0; }
extern "C" int is_system_bitmap(BITMAP *bmp) { (void)bmp; return 0; }
extern "C" int is_video_bitmap(BITMAP *bmp)  { (void)bmp; return 0; }

extern "C" void acquire_bitmap(BITMAP *bmp) { (void)bmp; }
extern "C" void release_bitmap(BITMAP *bmp) { (void)bmp; }
extern "C" void acquire_screen(void) {}
extern "C" void release_screen(void) {
    if (screen && screen->pixels) {
        esp32_flush_screen_from_allegro(screen->pixels, screen->w, screen->h);
    }
}
extern "C" void lock_bitmap(BITMAP *bmp) { (void)bmp; }

extern "C" void set_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2) {
    if (!bitmap) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    bitmap->cl = (x1 < 0) ? 0 : x1;
    bitmap->ct = (y1 < 0) ? 0 : y1;
    bitmap->cr = (x2 + 1 > bitmap->w) ? bitmap->w : (x2 + 1);
    bitmap->cb = (y2 + 1 > bitmap->h) ? bitmap->h : (y2 + 1);
}

extern "C" void set_clip_state(BITMAP *bitmap, int state) {
    if (!bitmap) return;
    bitmap->clip = state;
}

// ============================================================================
// Drawing Primitives
// ============================================================================
static inline void clip_bounds(const BITMAP *bmp, int *cl, int *ct, int *cr, int *cb) {
    if (bmp->clip) {
        *cl = (bmp->cl < 0) ? 0 : bmp->cl;
        *ct = (bmp->ct < 0) ? 0 : bmp->ct;
        *cr = (bmp->cr > bmp->w) ? bmp->w : bmp->cr;
        *cb = (bmp->cb > bmp->h) ? bmp->h : bmp->cb;
    } else {
        *cl = 0;
        *ct = 0;
        *cr = bmp->w;
        *cb = bmp->h;
    }
}

extern "C" void clear_to_color(BITMAP *bitmap, int color) {
    if (!bitmap || !bitmap->pixels) return;
    uint16_t c = (uint16_t)color;
    for (int y = 0; y < bitmap->h; y++) {
        uint16_t *row = bitmap->pixels + y * bitmap->stride;
        for (int x = 0; x < bitmap->w; x++) {
            row[x] = c;
        }
    }
}

extern "C" void clear_bitmap(BITMAP *bitmap) {
    clear_to_color(bitmap, 0);
}

extern "C" void putpixel(BITMAP *bmp, int x, int y, int color) {
    if (!bmp || !bmp->pixels) return;
    int cl, ct, cr, cb;
    clip_bounds(bmp, &cl, &ct, &cr, &cb);
    if (x < cl || x >= cr || y < ct || y >= cb) return;
    bmp->pixels[y * bmp->stride + x] = (uint16_t)color;
}

extern "C" int getpixel(BITMAP *bmp, int x, int y) {
    if (!bmp || !bmp->pixels) return -1;
    if (x < 0 || x >= bmp->w || y < 0 || y >= bmp->h) return -1;
    return (int)bmp->pixels[y * bmp->stride + x];
}

extern "C" void hline(BITMAP *bmp, int x1, int y, int x2, int color) {
    if (!bmp || !bmp->pixels) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    int cl, ct, cr, cb;
    clip_bounds(bmp, &cl, &ct, &cr, &cb);
    if (y < ct || y >= cb || x2 < cl || x1 >= cr) return;
    if (x1 < cl) x1 = cl;
    if (x2 >= cr) x2 = cr - 1;
    uint16_t *row = bmp->pixels + y * bmp->stride;
    uint16_t c = (uint16_t)color;
    for (int x = x1; x <= x2; x++) {
        row[x] = c;
    }
}

extern "C" void vline(BITMAP *bmp, int x, int y1, int y2, int color) {
    if (!bmp || !bmp->pixels) return;
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    int cl, ct, cr, cb;
    clip_bounds(bmp, &cl, &ct, &cr, &cb);
    if (x < cl || x >= cr || y2 < ct || y1 >= cb) return;
    if (y1 < ct) y1 = ct;
    if (y2 >= cb) y2 = cb - 1;
    uint16_t c = (uint16_t)color;
    for (int y = y1; y <= y2; y++) {
        bmp->pixels[y * bmp->stride + x] = c;
    }
}

extern "C" void line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        putpixel(bmp, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

extern "C" void rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color) {
    hline(bmp, x1, y1, x2, color);
    hline(bmp, x1, y2, x2, color);
    vline(bmp, x1, y1, y2, color);
    vline(bmp, x2, y1, y2, color);
}

extern "C" void rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color) {
    if (!bmp || !bmp->pixels) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    int cl, ct, cr, cb;
    clip_bounds(bmp, &cl, &ct, &cr, &cb);
    if (x2 < cl || x1 >= cr || y2 < ct || y1 >= cb) return;
    if (x1 < cl) x1 = cl;
    if (x2 >= cr) x2 = cr - 1;
    if (y1 < ct) y1 = ct;
    if (y2 >= cb) y2 = cb - 1;
    uint16_t c = (uint16_t)color;
    for (int y = y1; y <= y2; y++) {
        uint16_t *row = bmp->pixels + y * bmp->stride;
        for (int x = x1; x <= x2; x++) {
            row[x] = c;
        }
    }
}

extern "C" void circle(BITMAP *bmp, int x, int y, int radius, int color) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int cx = 0;
    int cy = radius;
    putpixel(bmp, x, y + radius, color);
    putpixel(bmp, x, y - radius, color);
    putpixel(bmp, x + radius, y, color);
    putpixel(bmp, x - radius, y, color);
    while (cx < cy) {
        if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
        cx++; ddF_x += 2; f += ddF_x;
        putpixel(bmp, x + cx, y + cy, color);
        putpixel(bmp, x - cx, y + cy, color);
        putpixel(bmp, x + cx, y - cy, color);
        putpixel(bmp, x - cx, y - cy, color);
        putpixel(bmp, x + cy, y + cx, color);
        putpixel(bmp, x - cy, y + cx, color);
        putpixel(bmp, x + cy, y - cx, color);
        putpixel(bmp, x - cy, y - cx, color);
    }
}

extern "C" void circlefill(BITMAP *bmp, int x, int y, int radius, int color) {
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        hline(bmp, x - dx, y + dy, x + dx, color);
    }
}

extern "C" void triangle(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    line(bmp, x1, y1, x2, y2, color);
    line(bmp, x2, y2, x3, y3, color);
    line(bmp, x3, y3, x1, y1, color);
}
extern "C" void polygon(BITMAP *bmp, int vertices, const int *points, int color) { (void)bmp; (void)vertices; (void)points; (void)color; }
extern "C" void ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int color) { (void)bmp; (void)x; (void)y; (void)rx; (void)ry; (void)color; }
extern "C" void ellipsefill(BITMAP *bmp, int x, int y, int rx, int ry, int color) { (void)bmp; (void)x; (void)y; (void)rx; (void)ry; (void)color; }
extern "C" void arc(BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color) { (void)bmp; (void)x; (void)y; (void)ang1; (void)ang2; (void)r; (void)color; }
extern "C" void spline(BITMAP *bmp, const int points[8], int color) { (void)bmp; (void)points; (void)color; }
extern "C" void floodfill(BITMAP *bmp, int x, int y, int color) { (void)bmp; (void)x; (void)y; (void)color; }
extern "C" void polygon3d(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D *vtx[]) { (void)bmp; (void)type; (void)texture; (void)vc; (void)vtx; }
extern "C" void polygon3d_f(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D_f *vtx[]) { (void)bmp; (void)type; (void)texture; (void)vc; (void)vtx; }
extern "C" void triangle3d(BITMAP *bmp, int type, BITMAP *tex, V3D *v1, V3D *v2, V3D *v3) { (void)bmp; (void)type; (void)tex; (void)v1; (void)v2; (void)v3; }
extern "C" void triangle3d_f(BITMAP *bmp, int type, BITMAP *tex, V3D_f *v1, V3D_f *v2, V3D_f *v3) { (void)bmp; (void)type; (void)tex; (void)v1; (void)v2; (void)v3; }
extern "C" void quad3d(BITMAP *bmp, int type, BITMAP *tex, V3D *v1, V3D *v2, V3D *v3, V3D *v4) { (void)bmp; (void)type; (void)tex; (void)v1; (void)v2; (void)v3; (void)v4; }
extern "C" void quad3d_f(BITMAP *bmp, int type, BITMAP *tex, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4) { (void)bmp; (void)type; (void)tex; (void)v1; (void)v2; (void)v3; (void)v4; }

// ============================================================================
// Blitting & Sprites
// ============================================================================
static void blit_internal(BITMAP *source, BITMAP *dest, int sx, int sy, int dx, int dy, int w, int h, int masked) {
    if (!source || !dest || !source->pixels || !dest->pixels) return;
    if (w <= 0 || h <= 0) return;

    if (sx < 0) { w += sx; dx -= sx; sx = 0; }
    if (sy < 0) { h += sy; dy -= sy; sy = 0; }
    if (sx + w > source->w) w = source->w - sx;
    if (sy + h > source->h) h = source->h - sy;

    int cl, ct, cr, cb;
    clip_bounds(dest, &cl, &ct, &cr, &cb);
    if (dx < cl) { int diff = cl - dx; w -= diff; sx += diff; dx = cl; }
    if (dy < ct) { int diff = ct - dy; h -= diff; sy += diff; dy = ct; }
    if (dx + w > cr) w = cr - dx;
    if (dy + h > cb) h = cb - dy;
    if (w <= 0 || h <= 0) return;

    for (int row = 0; row < h; row++) {
        const uint16_t *srcRow = source->pixels + (sy + row) * source->stride + sx;
        uint16_t *dstRow = dest->pixels + (dy + row) * dest->stride + dx;
        if (!masked) {
            memmove(dstRow, srcRow, (size_t)w * sizeof(uint16_t));
        } else {
            for (int col = 0; col < w; col++) {
                uint16_t c = srcRow[col];
                if (c != 0xF81Fu) {
                    dstRow[col] = c;
                }
            }
        }
    }
}

extern "C" void blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height) {
    blit_internal(source, dest, source_x, source_y, dest_x, dest_y, width, height, 0);
}

extern "C" void masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height) {
    blit_internal(source, dest, source_x, source_y, dest_x, dest_y, width, height, 1);
}

extern "C" void draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y) {
    if (!bmp || !sprite) return;
    blit_internal(sprite, bmp, 0, 0, x, y, sprite->w, sprite->h, 1);
}

extern "C" void draw_sprite_h_flip(BITMAP *bmp, BITMAP *sprite, int x, int y) {
    if (!bmp || !sprite || !bmp->pixels || !sprite->pixels) return;
    int cl, ct, cr, cb;
    clip_bounds(bmp, &cl, &ct, &cr, &cb);

    for (int sy = 0; sy < sprite->h; sy++) {
        int dy = y + sy;
        if (dy < ct || dy >= cb) continue;
        const uint16_t *srcRow = sprite->pixels + sy * sprite->stride;
        uint16_t *dstRow = bmp->pixels + dy * bmp->stride;
        for (int sx = 0; sx < sprite->w; sx++) {
            int dx = x + (sprite->w - 1 - sx);
            if (dx < cl || dx >= cr) continue;
            uint16_t c = srcRow[sx];
            if (c != 0xF81Fu) {
                dstRow[dx] = c;
            }
        }
    }
}

static void stretch_blit_internal(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h, int masked) {
    if (!s || !d || !s->pixels || !d->pixels) return;
    if (s_w <= 0 || s_h <= 0 || d_w <= 0 || d_h <= 0) return;

    // Fast path when 1:1 scale
    if (s_w == d_w && s_h == d_h) {
        blit_internal(s, d, s_x, s_y, d_x, d_y, d_w, d_h, masked);
        return;
    }

    int cl, ct, cr, cb;
    clip_bounds(d, &cl, &ct, &cr, &cb);

    int x_start = (d_x < cl) ? cl : d_x;
    int y_start = (d_y < ct) ? ct : d_y;
    int x_end = (d_x + d_w > cr) ? cr : (d_x + d_w);
    int y_end = (d_y + d_h > cb) ? cb : (d_y + d_h);

    for (int dy = y_start; dy < y_end; dy++) {
        int sy = s_y + ((dy - d_y) * s_h) / d_h;
        if (sy < 0 || sy >= s->h) continue;
        const uint16_t *srcRow = s->pixels + sy * s->stride;
        uint16_t *dstRow = d->pixels + dy * d->stride;
        for (int dx = x_start; dx < x_end; dx++) {
            int sx = s_x + ((dx - d_x) * s_w) / d_w;
            if (sx < 0 || sx >= s->w) continue;
            uint16_t c = srcRow[sx];
            if (!masked || c != 0xF81Fu) {
                dstRow[dx] = c;
            }
        }
    }
}

extern "C" void stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h) {
    stretch_blit_internal(s, d, s_x, s_y, s_w, s_h, d_x, d_y, d_w, d_h, 0);
}

extern "C" void masked_stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h) {
    stretch_blit_internal(s, d, s_x, s_y, s_w, s_h, d_x, d_y, d_w, d_h, 1);
}

extern "C" void draw_trans_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y) {
    draw_sprite(bmp, sprite, x, y);
}

extern "C" void draw_lit_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int color) {
    (void)color;
    draw_sprite(bmp, sprite, x, y);
}

extern "C" void draw_gouraud_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4) {
    (void)c1; (void)c2; (void)c3; (void)c4;
    draw_sprite(bmp, sprite, x, y);
}

extern "C" void set_trans_blender(int r, int g, int b, int a) { (void)r; (void)g; (void)b; (void)a; }
extern "C" void set_add_blender(int r, int g, int b, int a) { (void)r; (void)g; (void)b; (void)a; }

// ============================================================================
// Text & Font Rendering (Backed by EmbeddedAssetsData.h)
// ============================================================================
static const FONT g_embedded_font_alpha = {
    12, 32, 96, font_alpha_widths, font_alpha_rows
};

static const FONT g_embedded_font_score = {
    9, 48, 10, font_score_widths, font_score_rows
};

static DATAFILE g_embedded_fonts_datafile[3] = {
    { (void *)&g_embedded_font_alpha },
    { (void *)&g_embedded_font_score },
    { NULL }
};

extern "C" int text_height(const FONT *f) {
    return f ? f->height : 8;
}

extern "C" int text_length(const FONT *f, const char *str) {
    if (!f || !str) return 0;
    int len = 0;
    while (*str) {
        int ch = (unsigned char)(*str++);
        int idx = ch - f->first_char;
        if (idx >= 0 && idx < f->num_chars) {
            len += f->widths[idx];
        }
    }
    return len;
}

extern "C" void textout_ex(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int color, int bg) {
    if (!bmp || !f || !str) return;
    int curX = x;
    int h = f->height;
    while (*str) {
        int ch = (unsigned char)(*str++);
        int idx = ch - f->first_char;
        if (idx >= 0 && idx < f->num_chars) {
            int w = f->widths[idx];
            const uint32_t *glyphRows = f->rows + idx * h;
            for (int ry = 0; ry < h; ry++) {
                uint32_t bits = glyphRows[ry];
                for (int rx = 0; rx < w; rx++) {
                    if (bits & (1u << rx)) {
                        putpixel(bmp, curX + rx, y + ry, color);
                    } else if (bg >= 0) {
                        putpixel(bmp, curX + rx, y + ry, bg);
                    }
                }
            }
            curX += w;
        }
    }
}

extern "C" void textout_centre_ex(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int color, int bg) {
    int w = text_length(f, str);
    textout_ex(bmp, f, str, x - w / 2, y, color, bg);
}

extern "C" void textout_right_ex(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int color, int bg) {
    int w = text_length(f, str);
    textout_ex(bmp, f, str, x - w, y, color, bg);
}

extern "C" void textout_justify_ex(BITMAP *bmp, const FONT *f, const char *str, int x1, int x2, int y, int diff, int color, int bg) {
    (void)x2; (void)diff;
    textout_ex(bmp, f, str, x1, y, color, bg);
}

extern "C" int uvszprintf(char *buf, int size, const char *format, va_list args) {
    return vsnprintf(buf, (size_t)size, format, args);
}

// ============================================================================
// Embedded Asset Lookup (PNGs, Datafiles, .NWS Packfiles, WAVs, Tracker Music)
// ============================================================================
struct EmbeddedBitmapEntry {
    const char *filename;
    int w;
    int h;
    const uint16_t *pixels;
};

static const EmbeddedBitmapEntry g_embedded_bitmaps[] = {
    { "data/building0.png",     16,  52,  bmp_building0_pixels },
    { "data/building1.png",     20,  64,  bmp_building1_pixels },
    { "data/building2.png",     28,  45,  bmp_building2_pixels },
    { "data/building3.png",     48,  60,  bmp_building3_pixels },
    { "data/construction0.png", 16,  52,  bmp_construction0_pixels },
    { "data/credits.png",       200, 300, bmp_credits_pixels },
    { "data/firetruck.png",     72,  49,  bmp_firetruck_pixels },
    { "data/firetruckptr.png",  55,  19,  bmp_firetruckptr_pixels },
    { "data/gameover.png",      200, 300, bmp_gameover_pixels },
    { "data/heart.png",         7,   6,   bmp_heart_pixels },
    { "data/interface.png",     200, 300, bmp_interface_pixels },
    { "data/mask.png",          400, 400, bmp_mask_pixels },
    { "data/player.png",        128, 32,  bmp_player_pixels },
    { "data/playerptr.png",     44,  19,  bmp_playerptr_pixels },
    { "data/police.png",        24,  24,  bmp_police_pixels },
    { "data/policeptr.png",     45,  19,  bmp_policeptr_pixels },
    { "data/roads.png",         400, 400, bmp_roads_pixels },
    { "data/rubble.png",        116, 20,  bmp_rubble_pixels },
    { "data/title.png",         200, 300, bmp_title_pixels }
};

static const int g_embedded_bitmap_count = sizeof(g_embedded_bitmaps) / sizeof(g_embedded_bitmaps[0]);

extern "C" BITMAP *load_png(const char *filename, RGB *pal) {
    (void)pal;
    if (!filename) return NULL;
    for (int i = 0; i < g_embedded_bitmap_count; i++) {
        if (strcmp(filename, g_embedded_bitmaps[i].filename) == 0) {
            BITMAP *bmp = (BITMAP *)malloc(sizeof(BITMAP));
            if (!bmp) return NULL;
            bmp->w = g_embedded_bitmaps[i].w;
            bmp->h = g_embedded_bitmaps[i].h;
            bmp->stride = bmp->w;
            // Point directly to Flash .rodata to save ~809 KB of SRAM
            bmp->pixels = (uint16_t *)g_embedded_bitmaps[i].pixels;
            bmp->owns_memory = 0;
            bmp->cl = 0;
            bmp->ct = 0;
            bmp->cr = bmp->w;
            bmp->cb = bmp->h;
            bmp->clip = 1;
            return bmp;
        }
    }
    return NULL;
}

extern "C" BITMAP *load_bitmap(const char *filename, RGB *pal) {
    return load_png(filename, pal);
}

extern "C" int save_bmp(const char *filename, BITMAP *bmp, const RGB *pal) {
    (void)filename; (void)bmp; (void)pal;
    return 0;
}

extern "C" DATAFILE *load_datafile(const char *filename) {
    if (filename && strcmp(filename, "data/fonts.dat") == 0) {
        return g_embedded_fonts_datafile;
    }
    return NULL;
}

extern "C" void unload_datafile(DATAFILE *dat) {
    (void)dat;
}

extern "C" DATAFILE *load_datafile_object(const char *filename, const char *objectname) {
    (void)filename; (void)objectname;
    return NULL;
}

extern "C" void unload_datafile_object(DATAFILE *dat) {
    (void)dat;
}

extern "C" PACKFILE *pack_fopen(const char *filename, const char *mode) {
    (void)mode;
    if (!filename) return NULL;
    const uint8_t *data = NULL;
    uint32_t size = 0;
    if (strcmp(filename, "data/player.nws") == 0) {
        data = raw_player_nws_data;
        size = raw_player_nws_size;
    } else if (strcmp(filename, "data/fire.nws") == 0) {
        data = raw_fire_nws_data;
        size = raw_fire_nws_size;
    } else {
        return NULL;
    }
    PACKFILE *f = (PACKFILE *)malloc(sizeof(PACKFILE));
    if (!f) return NULL;
    f->data = data;
    f->size = size;
    f->pos = 0;
    return f;
}

extern "C" int pack_fclose(PACKFILE *f) {
    if (f) free(f);
    return 0;
}

extern "C" int pack_getc(PACKFILE *f) {
    if (!f || f->pos >= f->size) return EOF;
    return (int)f->data[f->pos++];
}

extern "C" int pack_igetw(PACKFILE *f) {
    int b1 = pack_getc(f);
    int b2 = pack_getc(f);
    if (b1 == EOF || b2 == EOF) return EOF;
    return (int)(int16_t)(b1 | (b2 << 8));
}

extern "C" long pack_igetl(PACKFILE *f) {
    int b1 = pack_getc(f);
    int b2 = pack_getc(f);
    int b3 = pack_getc(f);
    int b4 = pack_getc(f);
    if (b1 == EOF || b2 == EOF || b3 == EOF || b4 == EOF) return EOF;
    return (long)(int32_t)((uint32_t)b1 | ((uint32_t)b2 << 8) | ((uint32_t)b3 << 16) | ((uint32_t)b4 << 24));
}

// ============================================================================
// Embedded DUMB Tracker File System + Audio Mixer (Tracker Music + WAV SFX)
// ============================================================================
struct EmbeddedDumbMemFile {
    const uint8_t *data;
    dumb_off_t size;
    dumb_off_t pos;
};

static void *embedded_dumb_open(const char *filename) {
    if (!filename) return NULL;
    const uint8_t *data = NULL;
    uint32_t size = 0;
    if (strcmp(filename, "data/stalker_babylon.xm") == 0) {
        data = raw_stalker_babylon_xm_data;
        size = raw_stalker_babylon_xm_size;
    } else {
        return NULL;
    }
    EmbeddedDumbMemFile *mf = (EmbeddedDumbMemFile *)malloc(sizeof(EmbeddedDumbMemFile));
    if (!mf) return NULL;
    mf->data = data;
    mf->size = (dumb_off_t)size;
    mf->pos = 0;
    return mf;
}

static int embedded_dumb_skip(void *f, dumb_off_t n) {
    EmbeddedDumbMemFile *mf = (EmbeddedDumbMemFile *)f;
    if (mf->pos + n < 0 || mf->pos + n > mf->size) return -1;
    mf->pos += n;
    return 0;
}

static int embedded_dumb_getc(void *f) {
    EmbeddedDumbMemFile *mf = (EmbeddedDumbMemFile *)f;
    if (mf->pos >= mf->size) return -1;
    return (int)mf->data[mf->pos++];
}

static dumb_ssize_t embedded_dumb_getnc(char *ptr, size_t n, void *f) {
    EmbeddedDumbMemFile *mf = (EmbeddedDumbMemFile *)f;
    dumb_off_t rem = mf->size - mf->pos;
    if (rem <= 0) return 0;
    if ((dumb_off_t)n > rem) n = (size_t)rem;
    memcpy(ptr, mf->data + mf->pos, n);
    mf->pos += (dumb_off_t)n;
    return (dumb_ssize_t)n;
}

static void embedded_dumb_close(void *f) {
    if (f) free(f);
}

static int embedded_dumb_seek(void *f, dumb_off_t n) {
    EmbeddedDumbMemFile *mf = (EmbeddedDumbMemFile *)f;
    if (n < 0 || n > mf->size) return -1;
    mf->pos = n;
    return 0;
}

static dumb_off_t embedded_dumb_get_size(void *f) {
    EmbeddedDumbMemFile *mf = (EmbeddedDumbMemFile *)f;
    return mf->size;
}

static const DUMBFILE_SYSTEM g_embedded_dumb_dfs = {
    &embedded_dumb_open,
    &embedded_dumb_skip,
    &embedded_dumb_getc,
    &embedded_dumb_getnc,
    &embedded_dumb_close,
    &embedded_dumb_seek,
    &embedded_dumb_get_size
};

extern "C" void dumb_register_stdfiles(void) {
    register_dumbfile_system(&g_embedded_dumb_dfs);
}

// --- Active DUMB Player & SFX Mixer ---
struct AL_DUH_PLAYER {
    DUH_SIGRENDERER *sr;
    int n_channels;
    float volume;
    int freq;
};

static AL_DUH_PLAYER *g_active_duh_player = NULL;

struct SfxVoice {
    const SAMPLE *spl;
    uint32_t pos_fp;   // 16.16 fixed-point sample position
    uint32_t step_fp;  // 16.16 fixed-point step per output sample
    int vol;           // 0..255
    int loop;
    int active;
};

static SfxVoice g_voices[8];

static SAMPLE g_sample_fire  = { (int)sfx_fire_freq,  8, (unsigned long)sfx_fire_len,  sfx_fire_pcm };
static SAMPLE g_sample_siren = { (int)sfx_siren_freq, 8, (unsigned long)sfx_siren_len, sfx_siren_pcm };

extern "C" int install_sound(int digi, int midi, const char *cfg_path) {
    (void)digi; (void)midi; (void)cfg_path;
    memset(g_voices, 0, sizeof(g_voices));
    return 0;
}

extern "C" SAMPLE *load_sample(const char *filename) {
    if (!filename) return NULL;
    if (strcmp(filename, "data/fire.wav") == 0) {
        return &g_sample_fire;
    } else if (strcmp(filename, "data/siren.wav") == 0) {
        return &g_sample_siren;
    }
    return NULL;
}

extern "C" void destroy_sample(SAMPLE *spl) {
    (void)spl;
}

extern "C" int play_sample(const SAMPLE *spl, int vol, int pan, int freq, int loop) {
    (void)pan;
    if (!spl || !spl->data || spl->len == 0) return -1;
    for (int i = 0; i < 8; i++) {
        if (!g_voices[i].active) {
            g_voices[i].spl = spl;
            g_voices[i].pos_fp = 0;
            int eff_freq = (freq > 0) ? ((spl->freq * freq) / 1000) : spl->freq;
            if (eff_freq <= 0) eff_freq = 22050;
            g_voices[i].step_fp = (uint32_t)(((uint64_t)eff_freq << 16) / 22050u);
            g_voices[i].vol = (vol < 0) ? 0 : ((vol > 255) ? 255 : vol);
            g_voices[i].loop = loop;
            g_voices[i].active = 1;
            return i;
        }
    }
    return -1;
}

extern "C" void stop_sample(const SAMPLE *spl) {
    for (int i = 0; i < 8; i++) {
        if (g_voices[i].active && g_voices[i].spl == spl) {
            g_voices[i].active = 0;
        }
    }
}

extern "C" AL_DUH_PLAYER *al_start_duh(DUH *duh, int n_channels, long pos, float volume, long bufsize, int freq) {
    (void)bufsize;
    if (!duh) return NULL;
    AL_DUH_PLAYER *dp = (AL_DUH_PLAYER *)malloc(sizeof(AL_DUH_PLAYER));
    if (!dp) return NULL;
    dp->sr = duh_start_sigrenderer(duh, 0, n_channels, pos);
    if (!dp->sr) {
        free(dp);
        return NULL;
    }
    dp->n_channels = n_channels;
    dp->volume = volume;
    dp->freq = (freq > 0) ? freq : 22050;
    g_active_duh_player = dp;
    return dp;
}

extern "C" void al_stop_duh(AL_DUH_PLAYER *dp) {
    if (!dp) return;
    if (g_active_duh_player == dp) {
        g_active_duh_player = NULL;
    }
    if (dp->sr) {
        duh_end_sigrenderer(dp->sr);
    }
    free(dp);
}

extern "C" int al_poll_duh(AL_DUH_PLAYER *dp) {
    (void)dp;
    return 0;
}

extern "C" void esp32_render_audio_stream(int16_t *outStereoFrames, int frameCount, int sampleRate) {
    if (!outStereoFrames || frameCount <= 0) return;
    if (sampleRate <= 0) sampleRate = 22050;

    // 1. Render tracker music from DUMB if active
    AL_DUH_PLAYER *dp = g_active_duh_player;
    if (dp && dp->sr) {
        float delta = 65536.0f / (float)sampleRate;
        long rendered = duh_render(dp->sr, 16, 0, dp->volume, delta, frameCount, outStereoFrames);
        if (rendered < frameCount) {
            memset(outStereoFrames + rendered * 2, 0, (size_t)(frameCount - rendered) * 2 * sizeof(int16_t));
        }
    } else {
        memset(outStereoFrames, 0, (size_t)frameCount * 2 * sizeof(int16_t));
    }

    // 2. Mix active WAV sound effects (8-bit unsigned PCM -> 16-bit signed stereo)
    for (int v = 0; v < 8; v++) {
        if (!g_voices[v].active || !g_voices[v].spl) continue;
        const SAMPLE *spl = g_voices[v].spl;
        uint32_t step_fp = (uint32_t)(((uint64_t)spl->freq << 16) / (uint32_t)sampleRate);
        int vol = g_voices[v].vol;

        for (int f = 0; f < frameCount; f++) {
            uint32_t idx = g_voices[v].pos_fp >> 16;
            if (idx >= spl->len) {
                if (g_voices[v].loop) {
                    g_voices[v].pos_fp = 0;
                    idx = 0;
                } else {
                    g_voices[v].active = 0;
                    break;
                }
            }
            int s8 = (int)spl->data[idx] - 128;
            int s16 = (s8 * 256 * vol) / 255;

            int32_t left  = (int32_t)outStereoFrames[f * 2 + 0] + s16;
            int32_t right = (int32_t)outStereoFrames[f * 2 + 1] + s16;
            if (left > 32767) left = 32767; else if (left < -32768) left = -32768;
            if (right > 32767) right = 32767; else if (right < -32768) right = -32768;
            outStereoFrames[f * 2 + 0] = (int16_t)left;
            outStereoFrames[f * 2 + 1] = (int16_t)right;

            g_voices[v].pos_fp += step_fp;
        }
    }
}
