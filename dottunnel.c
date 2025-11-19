#include "dottunnel.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Lookup tables for tunnel calculation
static int *distance_table = NULL;
static int *angle_table = NULL;

// Dot spacing in texture space
#define DOT_SPACING 16
#define TEXTURE_SIZE 256

void dottunnel_init(void) {
    // Allocate lookup tables
    distance_table = (int *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
    if (!distance_table) {
        fprintf(stderr, "Failed to allocate dot tunnel distance table (%zu bytes)\n",
                SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
        return;
    }

    angle_table = (int *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
    if (!angle_table) {
        fprintf(stderr, "Failed to allocate dot tunnel angle table (%zu bytes)\n",
                SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
        free(distance_table);
        distance_table = NULL;
        return;
    }

    // Calculate distance and angle for each screen pixel
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int dx = x - center_x;
            int dy = y - center_y;

            // Distance calculation (with scaling)
            double dist = sqrt(dx * dx + dy * dy);
            distance_table[y * SCREEN_WIDTH + x] = (int)(32 * TEXTURE_SIZE / (dist + 1));

            // Angle calculation
            double angle = atan2(dy, dx);
            angle_table[y * SCREEN_WIDTH + x] = (int)(TEXTURE_SIZE * angle / (2 * M_PI)) & (TEXTURE_SIZE - 1);
        }
    }
}

void dottunnel_update(pixel_t *pixels, uint32_t time) {
    /* Check if initialization succeeded */
    if (!distance_table || !angle_table) {
        /* Clear screen to black on error */
        memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
        return;
    }

    // Clear screen to black
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixels[i] = RGB(0, 0, 0);
    }

    // Animation offsets
    int shift_x = (time / 48) & (TEXTURE_SIZE - 1);
    int shift_y = (time / 24) & (TEXTURE_SIZE - 1);

    // Color cycling
    int color_offset = time / 96;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;

            // Get precalculated distance and angle
            int d = distance_table[idx];
            int a = angle_table[idx];

            // Calculate texture coordinates with animation
            int tex_x = (d + shift_x) & (TEXTURE_SIZE - 1);
            int tex_y = (a + shift_y) & (TEXTURE_SIZE - 1);

            // Check if this position should have a dot
            if ((tex_x % DOT_SPACING) < 4 && (tex_y % DOT_SPACING) < 4) {
                // Calculate dot brightness based on depth (closer = brighter, center = darker)
                int brightness = 255 - ((d > 255) ? 255 : d);

                // Calculate color based on angle and distance
                int color_index = ((tex_x / DOT_SPACING) + (tex_y / DOT_SPACING) + color_offset) & 255;

                int r = (sine_table[color_index] + 128) * brightness / 255;
                int g = (sine_table[(color_index + 85) & 255] + 128) * brightness / 255;
                int b = (sine_table[(color_index + 170) & 255] + 128) * brightness / 255;

                // Draw dot with some size based on depth
                int dot_size = (brightness > 200) ? 2 : 1;

                for (int dy = 0; dy < dot_size; dy++) {
                    for (int dx = 0; dx < dot_size; dx++) {
                        int px = x + dx;
                        int py = y + dy;
                        if (px < SCREEN_WIDTH && py < SCREEN_HEIGHT) {
                            pixels[py * SCREEN_WIDTH + px] = RGB(r, g, b);
                        }
                    }
                }
            }
        }
    }
}

void dottunnel_cleanup(void) {
    if (distance_table) {
        free(distance_table);
        distance_table = NULL;
    }
    if (angle_table) {
        free(angle_table);
        angle_table = NULL;
    }
}
