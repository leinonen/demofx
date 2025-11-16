#include "plasma.h"
#include <math.h>

// Plasma palette
static pixel_t palette[256];

void plasma_init(void) {
    // Create a nice colorful palette
    for (int i = 0; i < 256; i++) {
        int r = (int)(128 + 127 * sin(i * M_PI / 32));
        int g = (int)(128 + 127 * sin(i * M_PI / 64));
        int b = (int)(128 + 127 * sin(i * M_PI / 128));
        palette[i] = RGB(r, g, b);
    }
}

void plasma_update(pixel_t *pixels, uint32_t time) {
    // Time-based animation variables
    int t = time / 16;  // Slow down the animation

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Classic plasma algorithm using sine waves
            int color = (
                sine_table[(x + t) & 255] +
                sine_table[(y + t) & 255] +
                sine_table[((x + y + t) / 2) & 255] +
                sine_table[((x * x + y * y) / 256 + t) & 255]
            ) / 4;

            pixels[y * SCREEN_WIDTH + x] = palette[color & 255];
        }
    }
}

void plasma_cleanup(void) {
    // Nothing to clean up
}
