#include "fire.h"
#include <stdlib.h>
#include <string.h>

// Fire buffer (extra row at bottom for fire source)
static uint8_t fire_buffer[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];

// Fire palette (black -> red -> orange -> yellow)
static pixel_t fire_palette[256];

void fire_init(void) {
    // Create fire palette - black -> red -> orange -> yellow, fading to black
    for (int i = 0; i < 256; i++) {
        int r, g, b;

        if (i < 85) {
            // Black to red
            r = i * 3;
            g = 0;
            b = 0;
        } else if (i < 170) {
            // Red to orange
            r = 255;
            g = (i - 85) * 3;
            b = 0;
        } else if (i < 255) {
            // Orange to yellow
            r = 255;
            g = 255;
            b = (i - 170) * 2;
        } else {
            // Cap at yellow (index 255)
            r = 255;
            g = 255;
            b = 170;
        }

        fire_palette[i] = RGB(r, g, b);
    }

    // Initialize fire buffer to zero
    memset(fire_buffer, 0, sizeof(fire_buffer));
}

void fire_update(pixel_t *pixels, uint32_t time) {
    // Add random fire source at the bottom
    int bottom_row = SCREEN_HEIGHT * SCREEN_WIDTH;
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        fire_buffer[bottom_row + x] = (rand() % 2) ? 255 : 0;
    }

    // Propagate fire upward with cooling
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;

            // Average the pixels below (with wrapping at edges)
            int x_left = (x - 1 + SCREEN_WIDTH) % SCREEN_WIDTH;
            int x_right = (x + 1) % SCREEN_WIDTH;

            int sum = fire_buffer[idx + SCREEN_WIDTH] +
                      fire_buffer[idx + SCREEN_WIDTH + x_left - x] +
                      fire_buffer[idx + SCREEN_WIDTH + x_right - x];

            // Cool down and propagate up
            int avg = sum / 3;
            int cooled = avg - (rand() % 3);
            // Threshold to fade cleanly to black (remove noise)
            fire_buffer[idx] = (cooled > 4) ? cooled : 0;

            // Draw to pixel buffer
            pixels[idx] = fire_palette[fire_buffer[idx]];
        }
    }
}

void fire_cleanup(void) {
    // Nothing to clean up
}
