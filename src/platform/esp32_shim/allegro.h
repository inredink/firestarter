#ifndef _ESP32_SHIM_ALLEGRO_H
#define _ESP32_SHIM_ALLEGRO_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AL_CONST const
#define END_OF_MAIN()
#define END_OF_FUNCTION(x)
#define LOCK_VARIABLE(x)
#define LOCK_FUNCTION(x)
#define BPS_TO_TIMER(x) (1000 / (x))

#ifndef ASSERT
#define ASSERT(x) ((void)0)
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef MID
#define MID(x,y,z) MAX((x), MIN((y), (z)))
#endif

typedef int32_t fixed;

typedef struct RGB {
	uint8_t r, g, b, filler;
} RGB;

typedef RGB PALETTE[256];

typedef struct BITMAP {
	int w, h;
	uint16_t *pixels;
	int stride;
	int owns_memory;
	int cl, ct, cr, cb;
	int clip;
} BITMAP;

typedef struct FONT {
	int height;
	int first_char;
	int num_chars;
	const uint8_t *widths;
	const uint32_t *rows;
} FONT;

typedef struct DATAFILE {
	void *dat;
} DATAFILE;

typedef struct SAMPLE {
	int freq;
	int bits;
	unsigned long len;
	const uint8_t *data;
} SAMPLE;

typedef struct PACKFILE {
	const uint8_t *data;
	uint32_t size;
	uint32_t pos;
} PACKFILE;

typedef struct V3D {
	fixed x, y, z, u, v;
	int c;
} V3D;

typedef struct V3D_f {
	float x, y, z, u, v;
	int c;
} V3D_f;

enum {
	KEY_UP = 1,
	KEY_DOWN = 2,
	KEY_LEFT = 3,
	KEY_RIGHT = 4,
	KEY_A = 5,
	KEY_Z = 6,
	KEY_ENTER = 7,
	KEY_RSHIFT = 8,
	KEY_ESC = 9,
	KEY_F2 = 10,
	KEY_F9 = 11,
	KEY_MAX = 32
};

#define GFX_AUTODETECT          0
#define GFX_AUTODETECT_WINDOWED 1
#define SWITCH_IN               0
#define SWITCH_OUT              1
#define SWITCH_PAUSE            0
#define SWITCH_BACKGROUND       1
#define DIGI_AUTODETECT         1
#define MIDI_NONE               0

typedef struct JOYSTICK_BUTTON_INFO {
	int b;
} JOYSTICK_BUTTON_INFO;

typedef struct JOYSTICK_AXIS_INFO {
	int pos, d1, d2;
} JOYSTICK_AXIS_INFO;

typedef struct JOYSTICK_STICK_INFO {
	JOYSTICK_AXIS_INFO axis[3];
} JOYSTICK_STICK_INFO;

typedef struct JOYSTICK_INFO {
	JOYSTICK_STICK_INFO stick[4];
	JOYSTICK_BUTTON_INFO button[16];
} JOYSTICK_INFO;

extern BITMAP *screen;
extern volatile char key[KEY_MAX];
extern JOYSTICK_INFO joy[4];

// Core system & timer
int allegro_init(void);
void allegro_exit(void);
void set_close_button_callback(void (*proc)(void));
void set_window_title(const char *name);
int install_keyboard(void);
int install_mouse(void);
int poll_keyboard(void);
int poll_mouse(void);
int install_int_ex(void (*proc)(void), int speed);
int install_int(void (*proc)(void), int speed);
void remove_int(void (*proc)(void));
void set_color_depth(int depth);
int set_gfx_mode(int card, int w, int h, int v_w, int v_h);
int set_display_switch_mode(int mode);
int set_display_switch_callback(int dir, void (*cb)(void));

// Color conversion (16-bit RGB565, magic pink = 0xF81F)
int makecol(int r, int g, int b);
int getr(int c);
int getg(int c);
int getb(int c);
int bitmap_mask_color(BITMAP *bmp);
int bitmap_color_depth(BITMAP *bmp);
void select_palette(const RGB *p);

// Bitmap management
BITMAP *create_bitmap(int width, int height);
BITMAP *create_bitmap_ex(int color_depth, int width, int height);
BITMAP *create_video_bitmap(int width, int height);
BITMAP *create_system_bitmap(int width, int height);
BITMAP *create_sub_bitmap(BITMAP *parent, int x, int y, int width, int height);
void destroy_bitmap(BITMAP *bitmap);
int is_same_bitmap(BITMAP *bmp1, BITMAP *bmp2);
int is_linear_bitmap(BITMAP *bmp);
int is_memory_bitmap(BITMAP *bmp);
int is_planar_bitmap(BITMAP *bmp);
int is_screen_bitmap(BITMAP *bmp);
int is_sub_bitmap(BITMAP *bmp);
int is_system_bitmap(BITMAP *bmp);
int is_video_bitmap(BITMAP *bmp);
void acquire_bitmap(BITMAP *bmp);
void release_bitmap(BITMAP *bmp);
void acquire_screen(void);
void release_screen(void);
void lock_bitmap(BITMAP *bmp);
void set_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2);
void set_clip_state(BITMAP *bitmap, int state);

// Drawing primitives
void clear_bitmap(BITMAP *bitmap);
void clear_to_color(BITMAP *bitmap, int color);
void putpixel(BITMAP *bmp, int x, int y, int color);
int getpixel(BITMAP *bmp, int x, int y);
void vline(BITMAP *bmp, int x, int y1, int y2, int color);
void hline(BITMAP *bmp, int x1, int y, int x2, int color);
void line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
void triangle(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color);
void polygon(BITMAP *bmp, int vertices, const int *points, int color);
void rect(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
void rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
void circle(BITMAP *bmp, int x, int y, int radius, int color);
void circlefill(BITMAP *bmp, int x, int y, int radius, int color);
void ellipse(BITMAP *bmp, int x, int y, int rx, int ry, int color);
void ellipsefill(BITMAP *bmp, int x, int y, int rx, int ry, int color);
void arc(BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color);
void spline(BITMAP *bmp, const int points[8], int color);
void floodfill(BITMAP *bmp, int x, int y, int color);
void polygon3d(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D *vtx[]);
void polygon3d_f(BITMAP *bmp, int type, BITMAP *texture, int vc, V3D_f *vtx[]);
void triangle3d(BITMAP *bmp, int type, BITMAP *tex, V3D *v1, V3D *v2, V3D *v3);
void triangle3d_f(BITMAP *bmp, int type, BITMAP *tex, V3D_f *v1, V3D_f *v2, V3D_f *v3);
void quad3d(BITMAP *bmp, int type, BITMAP *tex, V3D *v1, V3D *v2, V3D *v3, V3D *v4);
void quad3d_f(BITMAP *bmp, int type, BITMAP *tex, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4);

// Blitting & Sprites
void blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);
void masked_stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);
void draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y);
void draw_sprite_h_flip(BITMAP *bmp, BITMAP *sprite, int x, int y);
void draw_trans_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y);
void draw_lit_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int color);
void draw_gouraud_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4);
void set_trans_blender(int r, int g, int b, int a);
void set_add_blender(int r, int g, int b, int a);

// Text & Fonts
int text_length(const FONT *f, const char *str);
int text_height(const FONT *f);
void textout_ex(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int color, int bg);
void textout_centre_ex(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int color, int bg);
void textout_right_ex(BITMAP *bmp, const FONT *f, const char *str, int x, int y, int color, int bg);
void textout_justify_ex(BITMAP *bmp, const FONT *f, const char *str, int x1, int x2, int y, int diff, int color, int bg);
int uvszprintf(char *buf, int size, const char *format, va_list args);

// File, Datafile & Bitmap I/O (backed by EmbeddedAssetsData.h in Flash)
BITMAP *load_bitmap(const char *filename, RGB *pal);
int save_bmp(const char *filename, BITMAP *bmp, const RGB *pal);
DATAFILE *load_datafile(const char *filename);
void unload_datafile(DATAFILE *dat);
DATAFILE *load_datafile_object(const char *filename, const char *objectname);
void unload_datafile_object(DATAFILE *dat);
PACKFILE *pack_fopen(const char *filename, const char *mode);
int pack_fclose(PACKFILE *f);
int pack_getc(PACKFILE *f);
int pack_igetw(PACKFILE *f);
long pack_igetl(PACKFILE *f);

// Sound & Audio Mixer
int install_sound(int digi, int midi, const char *cfg_path);
SAMPLE *load_sample(const char *filename);
void destroy_sample(SAMPLE *spl);
int play_sample(const SAMPLE *spl, int vol, int pan, int freq, int loop);
void stop_sample(const SAMPLE *spl);

// Embedded audio rendering hook (renders mixed 16-bit signed stereo/mono PCM for I2S DMA)
void esp32_render_audio_stream(int16_t *outStereoFrames, int frameCount, int sampleRate);

#ifdef __cplusplus
}
#endif

#endif
