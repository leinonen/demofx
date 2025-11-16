#include "starfield.h"
#include <stdlib.h>
#include <string.h>

#define NUM_STARS 512

typedef struct {
    float x, y, z;
} Star;

static Star stars[NUM_STARS];

void starfield_init(void) {
    // Initialize stars at random positions
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = (float)(rand() % (SCREEN_WIDTH * 2)) - SCREEN_WIDTH;
        stars[i].y = (float)(rand() % (SCREEN_HEIGHT * 2)) - SCREEN_HEIGHT;
        stars[i].z = (float)(rand() % 256) + 1;
    }
}

void starfield_update(pixel_t *pixels, uint32_t time) {
    // Clear screen to black
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Speed of movement
    float speed = 2.0f;

    for (int i = 0; i < NUM_STARS; i++) {
        // Move star towards viewer
        stars[i].z -= speed;

        // Reset star if it passes the viewer
        if (stars[i].z <= 0) {
            stars[i].x = (float)(rand() % (SCREEN_WIDTH * 2)) - SCREEN_WIDTH;
            stars[i].y = (float)(rand() % (SCREEN_HEIGHT * 2)) - SCREEN_HEIGHT;
            stars[i].z = 256.0f;
        }

        // Project 3D position to 2D screen
        float k = 128.0f / stars[i].z;
        int screen_x = (int)(stars[i].x * k + SCREEN_WIDTH / 2);
        int screen_y = (int)(stars[i].y * k + SCREEN_HEIGHT / 2);

        // Check if star is visible on screen
        if (screen_x >= 0 && screen_x < SCREEN_WIDTH &&
            screen_y >= 0 && screen_y < SCREEN_HEIGHT) {

            // Calculate brightness based on depth (closer = brighter)
            int brightness = (int)(255 - stars[i].z);
            if (brightness < 0) brightness = 0;
            if (brightness > 255) brightness = 255;

            // Draw star
            pixel_t color = RGB(brightness, brightness, brightness);
            pixels[screen_y * SCREEN_WIDTH + screen_x] = color;

            // Draw larger stars for closer ones
            if (stars[i].z < 64) {
                // Draw a small cross pattern for bright stars
                if (screen_x > 0) {
                    pixels[screen_y * SCREEN_WIDTH + screen_x - 1] = color;
                }
                if (screen_x < SCREEN_WIDTH - 1) {
                    pixels[screen_y * SCREEN_WIDTH + screen_x + 1] = color;
                }
                if (screen_y > 0) {
                    pixels[(screen_y - 1) * SCREEN_WIDTH + screen_x] = color;
                }
                if (screen_y < SCREEN_HEIGHT - 1) {
                    pixels[(screen_y + 1) * SCREEN_WIDTH + screen_x] = color;
                }
            }
        }
    }
}

void starfield_cleanup(void) {
    // Nothing to clean up
}
