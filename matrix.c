#include "matrix.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_DROPS 50
#define MIN_SPEED 1
#define MAX_SPEED 4
#define MIN_LENGTH 8
#define MAX_LENGTH 25
#define SPAWN_CHANCE 3  // Percentage chance to spawn new drop each frame

typedef struct {
    int x;              // Column position
    int y;              // Head position
    int speed;          // Fall speed (pixels per frame)
    int length;         // Trail length
    int active;         // Is this drop active
    int brightness;     // Brightness variation
} MatrixDrop;

static MatrixDrop drops[NUM_DROPS];
static pixel_t palette[256];
static uint8_t brightness_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static int initialized = 0;

void matrix_init(void) {
    // Seed random number generator
    srand(time(NULL));

    // Create green color palette
    for (int i = 0; i < 256; i++) {
        int g = i;
        int r = 0;
        int b = 0;

        // Add some variation for brighter values
        if (i > 240) {
            // Very bright head - almost pure white
            g = 255;
            r = 200 + (i - 240);
            b = 200 + (i - 240);
        } else if (i > 200) {
            // Bright highlights - greenish white
            g = i;
            r = (i - 200) * 4;
            b = (i - 200) * 2;
        } else if (i > 128) {
            // Medium bright - pure green
            g = i;
        } else {
            // Dark green
            g = i;
            r = 0;
        }

        palette[i] = RGB(r, g, b);
    }

    // Initialize all drops as inactive
    for (int i = 0; i < NUM_DROPS; i++) {
        drops[i].active = 0;
        drops[i].x = rand() % SCREEN_WIDTH;
        drops[i].y = -(rand() % 200);  // Start above screen
        drops[i].speed = MIN_SPEED + (rand() % (MAX_SPEED - MIN_SPEED + 1));
        drops[i].length = MIN_LENGTH + (rand() % (MAX_LENGTH - MIN_LENGTH + 1));
        drops[i].brightness = 200 + (rand() % 56);  // 200-255
    }

    // Clear brightness buffer
    memset(brightness_buffer, 0, sizeof(brightness_buffer));

    // Activate some initial drops
    for (int i = 0; i < NUM_DROPS / 3; i++) {
        drops[i].active = 1;
    }

    initialized = 1;
}

void matrix_update(pixel_t *pixels, uint32_t time) {
    if (!initialized) {
        matrix_init();
    }

    // Fade the screen (creates trail effect)
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        if (brightness_buffer[i] > 0) {
            brightness_buffer[i] = (brightness_buffer[i] * 220) / 256;  // Fade to ~86%
        }
    }

    // Update and draw each drop
    for (int i = 0; i < NUM_DROPS; i++) {
        if (drops[i].active) {
            // Move drop down
            drops[i].y += drops[i].speed;

            // Draw the drop trail
            for (int j = 0; j < drops[i].length; j++) {
                int y_pos = drops[i].y - j;

                if (y_pos >= 0 && y_pos < SCREEN_HEIGHT &&
                    drops[i].x >= 0 && drops[i].x < SCREEN_WIDTH) {

                    int index = y_pos * SCREEN_WIDTH + drops[i].x;

                    // Calculate brightness (head is brightest)
                    int brightness;
                    if (j == 0) {
                        // Head - maximum brightness (almost pure white)
                        brightness = 255;
                    } else {
                        // Trail - fade from bright green to dark
                        brightness = drops[i].brightness - (j * drops[i].brightness / drops[i].length);
                        if (brightness < 0) brightness = 0;
                    }

                    // Update brightness buffer (keep max brightness)
                    if (brightness > brightness_buffer[index]) {
                        brightness_buffer[index] = brightness;
                    }
                }
            }

            // Check if drop is off screen
            if (drops[i].y - drops[i].length > SCREEN_HEIGHT) {
                drops[i].active = 0;
            }
        } else {
            // Randomly spawn new drops
            if (rand() % 100 < SPAWN_CHANCE) {
                drops[i].active = 1;
                drops[i].x = rand() % SCREEN_WIDTH;
                drops[i].y = 0;
                drops[i].speed = MIN_SPEED + (rand() % (MAX_SPEED - MIN_SPEED + 1));
                drops[i].length = MIN_LENGTH + (rand() % (MAX_LENGTH - MIN_LENGTH + 1));
                drops[i].brightness = 200 + (rand() % 56);  // 200-255
            }
        }
    }

    // Render brightness buffer to screen with palette
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixels[i] = palette[brightness_buffer[i]];
    }

    // Add some random "glitches" - occasional bright pixels
    if (rand() % 100 < 5) {
        int glitch_x = rand() % SCREEN_WIDTH;
        int glitch_y = rand() % SCREEN_HEIGHT;
        int glitch_index = glitch_y * SCREEN_WIDTH + glitch_x;
        brightness_buffer[glitch_index] = 200 + (rand() % 56);
    }
}

void matrix_cleanup(void) {
    // Nothing to clean up (all static memory)
    initialized = 0;
}
