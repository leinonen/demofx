#include "ripple.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int *ripple_buffer1;
static int *ripple_buffer2;
static int *current_buffer;
static int *previous_buffer;
static pixel_t *background_texture;

#define DAMPING 5  /* Damping shift amount (higher = more damping), typical: 4-8 */

static void create_background_texture(void) {
    /* Create a simple checkerboard pattern to distort */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int checker = ((x / 20) + (y / 20)) & 1;

            /* Add some color variation with sine waves for visual interest */
            int r = 64 + sine_table[(x * 2) & 255] / 2048 + (checker ? 100 : 0);
            int g = 128 + sine_table[(y * 3) & 255] / 2048 + (checker ? 0 : 80);
            int b = 180 + sine_table[((x + y) * 2) & 255] / 2048;

            /* Clamp values */
            r = (r < 0) ? 0 : (r > 255) ? 255 : r;
            g = (g < 0) ? 0 : (g > 255) ? 255 : g;
            b = (b < 0) ? 0 : (b > 255) ? 255 : b;

            background_texture[y * SCREEN_WIDTH + x] = RGB(r, g, b);
        }
    }
}

void ripple_init(void) {
    int buffer_size = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int);

    ripple_buffer1 = (int *)malloc(buffer_size);
    ripple_buffer2 = (int *)malloc(buffer_size);
    background_texture = (pixel_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    /* Initialize buffers to zero (calm water) */
    memset(ripple_buffer1, 0, buffer_size);
    memset(ripple_buffer2, 0, buffer_size);

    current_buffer = ripple_buffer1;
    previous_buffer = ripple_buffer2;

    create_background_texture();
}

static void make_ripple(int x, int y, int radius, int height) {
    /* Create a circular disturbance */
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int dist_sq = dx * dx + dy * dy;
            if (dist_sq <= radius * radius) {
                int px = x + dx;
                int py = y + dy;

                if (px >= 1 && px < SCREEN_WIDTH - 1 && py >= 1 && py < SCREEN_HEIGHT - 1) {
                    /* Gaussian-like falloff */
                    int dist = (int)sqrt(dist_sq);
                    int ripple_height = height * (radius - dist) / radius;
                    current_buffer[py * SCREEN_WIDTH + px] = ripple_height;
                }
            }
        }
    }
}

void ripple_update(pixel_t *pixels, uint32_t time) {
    /* Create automatic ripples at intervals */
    if (time % 800 == 0) {
        int x = 80 + (sine_table[(time / 8) & 255] / 256);
        int y = 100 + (cosine_table[(time / 12) & 255] / 256);
        make_ripple(x, y, 3, 512);
    }

    if (time % 1100 == 0) {
        int x = 240 - (sine_table[(time / 10) & 255] / 256);
        int y = 100 + (sine_table[(time / 15) & 255] / 256);
        make_ripple(x, y, 4, 384);
    }

    if (time % 1500 == 0) {
        int x = 160;
        int y = 100 + (sine_table[(time / 20) & 255] / 300);
        make_ripple(x, y, 5, 450);
    }

    /* Water ripple simulation: propagate waves */
    for (int y = 1; y < SCREEN_HEIGHT - 1; y++) {
        for (int x = 1; x < SCREEN_WIDTH - 1; x++) {
            int idx = y * SCREEN_WIDTH + x;

            /* Average of 4 neighboring cells */
            int sum = current_buffer[idx - 1] +           /* left */
                      current_buffer[idx + 1] +           /* right */
                      current_buffer[idx - SCREEN_WIDTH] + /* top */
                      current_buffer[idx + SCREEN_WIDTH];  /* bottom */

            /* Water wave formula: new = (average * 2) - previous */
            /* Then apply damping to prevent infinite oscillation */
            sum = (sum >> 1) - previous_buffer[idx];
            sum = sum - (sum >> DAMPING);

            previous_buffer[idx] = sum;
        }
    }

    /* Swap buffers */
    int *temp = current_buffer;
    current_buffer = previous_buffer;
    previous_buffer = temp;

    /* Render: use ripple heights to displace texture lookups */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            int height = current_buffer[idx];

            /* Calculate water normal/displacement from height differences */
            int xdiff = 0, ydiff = 0;
            if (x > 0 && x < SCREEN_WIDTH - 1) {
                xdiff = current_buffer[idx - 1] - current_buffer[idx + 1];
            }
            if (y > 0 && y < SCREEN_HEIGHT - 1) {
                ydiff = current_buffer[idx - SCREEN_WIDTH] - current_buffer[idx + SCREEN_WIDTH];
            }

            /* Apply displacement to texture coordinates */
            int offset_x = x + (xdiff >> 4);  /* Scale down displacement */
            int offset_y = y + (ydiff >> 4);

            /* Clamp to texture bounds */
            if (offset_x < 0) offset_x = 0;
            if (offset_x >= SCREEN_WIDTH) offset_x = SCREEN_WIDTH - 1;
            if (offset_y < 0) offset_y = 0;
            if (offset_y >= SCREEN_HEIGHT) offset_y = SCREEN_HEIGHT - 1;

            /* Sample the background texture with displacement */
            pixel_t color = background_texture[offset_y * SCREEN_WIDTH + offset_x];

            /* Add slight brightness variation based on height (water reflections) */
            int r = (color >> 16) & 0xFF;
            int g = (color >> 8) & 0xFF;
            int b = color & 0xFF;

            /* Modulate brightness by water height */
            int brightness = 128 + (height >> 3);
            if (brightness < 64) brightness = 64;
            if (brightness > 192) brightness = 192;

            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;

            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;

            pixels[idx] = RGB(r, g, b);
        }
    }
}

void ripple_cleanup(void) {
    free(ripple_buffer1);
    free(ripple_buffer2);
    free(background_texture);
}
