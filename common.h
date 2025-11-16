#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <math.h>

// Screen dimensions (classic Mode 13h resolution)
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200

// Window scale factor (for modern displays)
#define SCALE_FACTOR 3

// Pixel buffer type
typedef uint32_t pixel_t;

// Color manipulation macros (RGBA8888 format for SDL)
#define RGB(r, g, b) (((r) << 24) | ((g) << 16) | ((b) << 8) | 0xFF)
#define RGBA(r, g, b, a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))

// Math constants
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Common sine table for effects (0-255 maps to 0-2*PI)
extern int sine_table[256];
extern int cosine_table[256];

// Initialize common lookup tables
void init_tables(void);

// Draw a line using Bresenham's algorithm
void draw_line(pixel_t *pixels, int x0, int y0, int x1, int y1, pixel_t color);

#endif // COMMON_H
