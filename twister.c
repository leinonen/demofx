#include "twister.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Texture size (must be power of 2 for fast wrapping)
#define TEXTURE_SIZE 256

// Twister texture
static pixel_t texture[TEXTURE_SIZE * TEXTURE_SIZE];

// Create a visually interesting texture for the twister
void twister_init(void) {
    // Create vertical stripes with horizontal gradient
    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            // Create vertical stripes (repeating pattern)
            int stripe = (x / 8) & 7; // 8 stripes repeating

            // Add some smooth horizontal gradient based on Y
            double y_gradient = sin(y * M_PI / 64.0);

            // Create color based on stripe position and Y gradient
            int r, g, b;

            if (stripe < 4) {
                // First half: Purple to cyan gradient
                double t = stripe / 4.0;
                r = (int)(128 + 64 * t + y_gradient * 32);
                g = (int)(32 + 96 * t + y_gradient * 64);
                b = (int)(192 + 32 * t + y_gradient * 32);
            } else {
                // Second half: Cyan to yellow gradient
                double t = (stripe - 4) / 4.0;
                r = (int)(192 + 63 * t - y_gradient * 32);
                g = (int)(128 + 96 * t + y_gradient * 32);
                b = (int)(224 - 96 * t - y_gradient * 64);
            }

            // Add some texture detail
            if ((x % 8) == 0 || (y % 16) == 0) {
                r = (r * 3) / 4;
                g = (g * 3) / 4;
                b = (b * 3) / 4;
            }

            texture[y * TEXTURE_SIZE + x] = RGB(r & 255, g & 255, b & 255);
        }
    }
}

void twister_update(pixel_t *pixels, uint32_t time) {
    // Animation parameters
    double time_factor = time * 0.001; // Convert to seconds

    // Twister bar width
    int bar_width = 140;
    int center_x = SCREEN_WIDTH / 2;

    // Render each screen row
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        // Calculate twist rotation for this row (more twist at edges)
        double twist_progress = (double)y / SCREEN_HEIGHT;
        double twist_angle = time_factor + twist_progress * 3.0 * M_PI;

        // Combine multiple sine waves for organic horizontal movement
        int sine_idx1 = (y * 2 + time / 4) & 255;
        int sine_idx2 = (y * 3 + time / 6) & 255;
        int sine_idx3 = (y + time / 8) & 255;

        int wave1 = (sine_table[sine_idx1] - 128) / 4;
        int wave2 = (sine_table[sine_idx2] - 128) / 6;
        int wave3 = (sine_table[sine_idx3] - 128) / 10;

        int sine_offset = wave1 + wave2 + wave3;

        // Render each column for this row
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Calculate distance from center
            int dx = x - center_x - sine_offset;

            // Map screen X to a circular angle around the twister bar
            // This creates the cylindrical/column effect
            double angle = (double)dx / (bar_width / 2.0) * M_PI;

            // Check if we're within the visible bar
            if (angle < -M_PI/2 || angle > M_PI/2) {
                pixels[y * SCREEN_WIDTH + x] = RGB(0, 0, 0);
                continue;
            }

            // Apply twist rotation to the angle
            double rotated_angle = angle + twist_angle;

            // Convert angle to texture U coordinate
            int tex_u = ((int)(rotated_angle * 64.0 / M_PI) + time / 8) & (TEXTURE_SIZE - 1);

            // V coordinate is simply the Y position
            int tex_v = ((y * 2 + time / 4) & (TEXTURE_SIZE - 1));

            // Add depth shading based on angle (simulate cylinder)
            double brightness = 0.6 + 0.4 * cos(angle);

            // Sample texture
            pixel_t color = texture[tex_v * TEXTURE_SIZE + tex_u];

            // Apply brightness
            int r = ((color >> 24) & 0xFF) * brightness;
            int g = ((color >> 16) & 0xFF) * brightness;
            int b = ((color >> 8) & 0xFF) * brightness;

            pixels[y * SCREEN_WIDTH + x] = RGB(r, g, b);
        }
    }
}

void twister_cleanup(void) {
    // No dynamic allocation, nothing to clean up
}
