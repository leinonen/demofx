#ifndef FONT_H
#define FONT_H

#include "common.h"

// Draw a single character at pixel coordinates
void draw_char(pixel_t *pixels, int x, int y, char c, pixel_t color);

// Draw a string at pixel coordinates
void draw_text(pixel_t *pixels, int x, int y, const char *text, pixel_t color);

// Draw text with a shadow/glow effect for retro look
void draw_text_with_glow(pixel_t *pixels, int x, int y, const char *text, pixel_t color, pixel_t glow_color);

#endif // FONT_H
