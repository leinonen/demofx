#include "raster.h"
#include <stdlib.h>
#include <string.h>

// Number of raster bars
#define NUM_BARS 8
#define BAR_HEIGHT 12

// Raster bar colors (classic Amiga-style palette)
static pixel_t bar_colors[NUM_BARS];

void raster_init(void) {
    // Initialize with classic colorful bars
    bar_colors[0] = RGB(255, 0, 0);     // Red
    bar_colors[1] = RGB(255, 128, 0);   // Orange
    bar_colors[2] = RGB(255, 255, 0);   // Yellow
    bar_colors[3] = RGB(0, 255, 0);     // Green
    bar_colors[4] = RGB(0, 255, 255);   // Cyan
    bar_colors[5] = RGB(0, 128, 255);   // Light Blue
    bar_colors[6] = RGB(128, 0, 255);   // Purple
    bar_colors[7] = RGB(255, 0, 255);   // Magenta
}

void raster_update(pixel_t *pixels, uint32_t time) {
    // Clear screen to black
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Time-based animation
    int t = time / 8;

    // Draw each raster bar with sinusoidal movement
    for (int bar = 0; bar < NUM_BARS; bar++) {
        // Calculate vertical position with sine wave
        // Each bar has a different phase offset
        int phase = bar * 32;
        int wave_offset = (sine_table[(t + phase) & 255] - 128) / 2;
        int y_center = 40 + bar * 20 + wave_offset;

        // Draw the straight horizontal bar with gradient fade on edges
        for (int dy = 0; dy < BAR_HEIGHT; dy++) {
            int y = y_center - BAR_HEIGHT/2 + dy;

            if (y >= 0 && y < SCREEN_HEIGHT) {
                // Calculate brightness based on distance from center
                int dist_from_center = abs(dy - BAR_HEIGHT/2);
                float brightness = 1.0f - (float)dist_from_center / (BAR_HEIGHT/2);
                brightness = brightness * brightness; // Square for nicer falloff

                // Extract RGB components
                uint8_t r = (bar_colors[bar] >> 24) & 0xFF;
                uint8_t g = (bar_colors[bar] >> 16) & 0xFF;
                uint8_t b = (bar_colors[bar] >> 8) & 0xFF;

                // Apply brightness
                r = (uint8_t)(r * brightness);
                g = (uint8_t)(g * brightness);
                b = (uint8_t)(b * brightness);

                pixel_t color = RGB(r, g, b);

                // Draw the entire horizontal line
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    pixels[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }
}

void raster_cleanup(void) {
    // Nothing to clean up
}
