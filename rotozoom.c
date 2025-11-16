#include "rotozoom.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Texture size (must be power of 2 for fast wrapping)
#define TEXTURE_SIZE 256

// Rotozoom texture
static pixel_t texture[TEXTURE_SIZE * TEXTURE_SIZE];

// Create a visually interesting texture for the rotozoom
void rotozoom_init(void) {
    // Create a checkerboard pattern with gradient overlay
    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            // Checkerboard pattern
            int check_size = 32;
            int check = ((x / check_size) + (y / check_size)) & 1;

            // Add some circular gradient from center
            int dx = x - TEXTURE_SIZE / 2;
            int dy = y - TEXTURE_SIZE / 2;
            double dist = sqrt(dx * dx + dy * dy);

            // Create color based on pattern
            if (check) {
                int r = (int)(128 + 64 * sin(dist * 0.1));
                int g = (int)(128 + 64 * cos(dist * 0.1));
                int b = (int)(192 + 63 * sin(x * 0.05));
                texture[y * TEXTURE_SIZE + x] = RGB(r & 255, g & 255, b & 255);
            } else {
                int r = (int)(64 + 32 * sin(dist * 0.1));
                int g = (int)(64 + 32 * cos(dist * 0.1));
                int b = (int)(96 + 32 * cos(y * 0.05));
                texture[y * TEXTURE_SIZE + x] = RGB(r & 255, g & 255, b & 255);
            }
        }
    }
}

void rotozoom_update(pixel_t *pixels, uint32_t time) {
    // Animation parameters
    double time_factor = time * 0.001; // Convert to seconds

    // Calculate rotation angle (oscillates smoothly)
    double rotation = time_factor * 0.5;

    // Calculate zoom factor (oscillates between 0.5 and 2.0)
    double zoom = 1.0 + 0.8 * sin(time_factor * 0.3);

    // Precalculate rotation values
    double cos_rot = cos(rotation);
    double sin_rot = sin(rotation);

    // Screen center
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    // Render each pixel
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Convert screen coordinates to centered coordinates
            double px = (x - center_x);
            double py = (y - center_y);

            // Apply rotation and zoom
            double rotated_x = (px * cos_rot - py * sin_rot) / zoom;
            double rotated_y = (px * sin_rot + py * cos_rot) / zoom;

            // Convert to texture coordinates and add scrolling
            int tex_x = ((int)(rotated_x + time / 8)) & (TEXTURE_SIZE - 1);
            int tex_y = ((int)(rotated_y + time / 8)) & (TEXTURE_SIZE - 1);

            // Sample texture
            pixels[y * SCREEN_WIDTH + x] = texture[tex_y * TEXTURE_SIZE + tex_x];
        }
    }
}

void rotozoom_cleanup(void) {
    // No dynamic allocation, nothing to clean up
}
