#include "tunnel.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Lookup tables for tunnel calculation
static int *distance_table = NULL;
static int *angle_table = NULL;
static uint8_t *fog_table = NULL;

// Tunnel texture size
#define TEXTURE_SIZE 256

// Tunnel texture
static pixel_t texture[TEXTURE_SIZE * TEXTURE_SIZE];

void tunnel_init(void) {
    // Allocate lookup tables
    distance_table = (int *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
    if (!distance_table) {
        fprintf(stderr, "Failed to allocate tunnel distance table (%zu bytes)\n",
                SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
        return;
    }

    angle_table = (int *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
    if (!angle_table) {
        fprintf(stderr, "Failed to allocate tunnel angle table (%zu bytes)\n",
                SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(int));
        free(distance_table);
        distance_table = NULL;
        return;
    }

    // Calculate distance and angle for each screen pixel
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    fog_table = (uint8_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT);
    if (!fog_table) {
        fprintf(stderr, "Failed to allocate tunnel fog table\n");
        free(distance_table);
        free(angle_table);
        distance_table = NULL;
        angle_table = NULL;
        return;
    }

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

            // Fog: center = dark (far), edges = bright (near)
            int fog = (int)(dist * 255 / 100);
            fog_table[y * SCREEN_WIDTH + x] = (uint8_t)(fog > 255 ? 255 : fog);
        }
    }

    // Create a checkerboard-style tunnel texture
    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            // Create XOR pattern with some color variation
            int val = (x ^ y);
            int r = val & 255;
            int g = (val * 2) & 255;
            int b = (val / 2) & 255;
            texture[y * TEXTURE_SIZE + x] = RGB(r, g, b);
        }
    }
}

void tunnel_update(pixel_t *pixels, uint32_t time) {
    /* Check if initialization succeeded */
    if (!distance_table || !angle_table) {
        /* Clear screen to black on error */
        memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
        return;
    }

    // Animation offsets
    int shift_x = (time / 8) & (TEXTURE_SIZE - 1);
    int shift_y = (time / 8) & (TEXTURE_SIZE - 1);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;

            // Get precalculated distance and angle
            int d = distance_table[idx];
            int a = angle_table[idx];

            // Calculate texture coordinates with animation
            int tex_x = (d + shift_x) & (TEXTURE_SIZE - 1);
            int tex_y = (a + shift_y) & (TEXTURE_SIZE - 1);

            // Sample texture with depth fog
            pixel_t p = texture[tex_y * TEXTURE_SIZE + tex_x];
            int fog = fog_table[idx];
            int r = ((p >> 24) & 0xFF) * fog / 255;
            int g = ((p >> 16) & 0xFF) * fog / 255;
            int b = ((p >> 8) & 0xFF) * fog / 255;
            pixels[idx] = RGB(r, g, b);
        }
    }
}

void tunnel_cleanup(void) {
    if (distance_table) {
        free(distance_table);
        distance_table = NULL;
    }
    if (angle_table) {
        free(angle_table);
        angle_table = NULL;
    }
    if (fog_table) {
        free(fog_table);
        fog_table = NULL;
    }
}
