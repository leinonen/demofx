#include "lens.h"
#include <math.h>
#include <string.h>

#define LENS_RADIUS 40
#define MAGNIFICATION 2.0f
#define CHECKER_SIZE 16
#define NUM_LENSES 3

// Background buffer for the checkerboard pattern
static pixel_t background[SCREEN_WIDTH * SCREEN_HEIGHT];

// Generate checkerboard pattern
static void generate_checkerboard(void) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int checker_x = x / CHECKER_SIZE;
            int checker_y = y / CHECKER_SIZE;
            int is_white = (checker_x + checker_y) & 1;

            pixel_t color;
            if (is_white) {
                color = RGB(220, 220, 220);
            } else {
                color = RGB(60, 60, 80);
            }

            background[y * SCREEN_WIDTH + x] = color;
        }
    }
}

void lens_init(void) {
    generate_checkerboard();
}

void lens_update(pixel_t *pixels, uint32_t time) {
    // Copy background to screen
    memcpy(pixels, background, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    float t = time * 0.001f; // Convert to seconds

    // Calculate positions for multiple lenses with different movement patterns
    float lens_x[NUM_LENSES];
    float lens_y[NUM_LENSES];

    // Lens 0: Figure-8 pattern (Lissajous curve)
    float offset_x = (cosine_table[(int)(t * 20.0f) & 255] - 128) / 127.0f;
    float offset_y = (sine_table[(int)(t * 40.0f) & 255] - 128) / 127.0f;
    lens_x[0] = SCREEN_WIDTH / 2.0f + offset_x * 80.0f;
    lens_y[0] = SCREEN_HEIGHT / 2.0f + offset_y * 60.0f;

    // Lens 1: Circular pattern (clockwise)
    float angle1 = t * 0.8f;
    float cos1 = (cosine_table[(int)(angle1 * 40.0f) & 255] - 128) / 127.0f;
    float sin1 = (sine_table[(int)(angle1 * 40.0f) & 255] - 128) / 127.0f;
    lens_x[1] = SCREEN_WIDTH / 2.0f + cos1 * 90.0f;
    lens_y[1] = SCREEN_HEIGHT / 2.0f + sin1 * 70.0f;

    // Lens 2: Horizontal oscillation with vertical wave
    float cos2 = (cosine_table[(int)(t * 30.0f) & 255] - 128) / 127.0f;
    float sin2 = (sine_table[(int)(t * 50.0f) & 255] - 128) / 127.0f;
    lens_x[2] = SCREEN_WIDTH / 2.0f + cos2 * 100.0f;
    lens_y[2] = SCREEN_HEIGHT / 2.0f + sin2 * 40.0f;

    // Apply lens distortion for each pixel
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Find the closest lens affecting this pixel
            float closest_dist = LENS_RADIUS;
            int closest_lens = -1;

            for (int i = 0; i < NUM_LENSES; i++) {
                float dx = x - lens_x[i];
                float dy = y - lens_y[i];
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < LENS_RADIUS && dist < closest_dist) {
                    closest_dist = dist;
                    closest_lens = i;
                }
            }

            // Apply distortion if pixel is inside a lens
            if (closest_lens >= 0) {
                float dx = x - lens_x[closest_lens];
                float dy = y - lens_y[closest_lens];
                float dist = closest_dist;

                // Calculate distortion amount (stronger at center)
                float dist_norm = dist / LENS_RADIUS;
                float distortion = 1.0f - dist_norm * dist_norm; // Smooth falloff

                // Sample from magnified position
                float mag_factor = 1.0f - distortion * (1.0f - 1.0f / MAGNIFICATION);
                int src_x = (int)(lens_x[closest_lens] + dx * mag_factor);
                int src_y = (int)(lens_y[closest_lens] + dy * mag_factor);

                // Clamp to screen bounds and apply with edge blending
                if (src_x >= 0 && src_x < SCREEN_WIDTH &&
                    src_y >= 0 && src_y < SCREEN_HEIGHT) {

                    pixel_t distorted_pixel = background[src_y * SCREEN_WIDTH + src_x];

                    // Smooth edge blending (fade out near the edge)
                    if (dist > LENS_RADIUS - 5.0f) {
                        float edge_blend = (LENS_RADIUS - dist) / 5.0f;
                        edge_blend = edge_blend * edge_blend; // Smooth curve

                        pixel_t bg_pixel = background[y * SCREEN_WIDTH + x];

                        // Blend distorted and background
                        int r_dist = (distorted_pixel >> 24) & 0xFF;
                        int g_dist = (distorted_pixel >> 16) & 0xFF;
                        int b_dist = (distorted_pixel >> 8) & 0xFF;

                        int r_bg = (bg_pixel >> 24) & 0xFF;
                        int g_bg = (bg_pixel >> 16) & 0xFF;
                        int b_bg = (bg_pixel >> 8) & 0xFF;

                        int r = (int)(r_dist * edge_blend + r_bg * (1.0f - edge_blend));
                        int g = (int)(g_dist * edge_blend + g_bg * (1.0f - edge_blend));
                        int b = (int)(b_dist * edge_blend + b_bg * (1.0f - edge_blend));

                        pixels[y * SCREEN_WIDTH + x] = RGB(r, g, b);
                    } else {
                        pixels[y * SCREEN_WIDTH + x] = distorted_pixel;
                    }
                }
            }
        }
    }
}

void lens_cleanup(void) {
    // Nothing to clean up (using static memory)
}
