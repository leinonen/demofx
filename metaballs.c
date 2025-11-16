#include "metaballs.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Number of metaballs
#define NUM_BALLS 6

// Metaball structure
typedef struct {
    double x, y;        // Position
    double radius;      // Influence radius
    double speed_x;     // Movement speed
    double speed_y;
    double phase_x;     // Phase offset for movement
    double phase_y;
} Metaball;

// Array of metaballs
static Metaball balls[NUM_BALLS];

// Color palette for intensity mapping
static pixel_t palette[256];

void metaballs_init(void) {
    // Initialize metaballs with different sizes and speeds
    for (int i = 0; i < NUM_BALLS; i++) {
        balls[i].x = SCREEN_WIDTH / 2;
        balls[i].y = SCREEN_HEIGHT / 2;
        balls[i].radius = 30.0 + (i * 10.0); // Varying sizes
        balls[i].speed_x = 0.3 + (i * 0.1);
        balls[i].speed_y = 0.25 + (i * 0.15);
        balls[i].phase_x = i * M_PI / 3.0; // Different phase offsets
        balls[i].phase_y = i * M_PI / 4.0;
    }

    // Create a color palette based on intensity
    for (int i = 0; i < 256; i++) {
        double t = i / 255.0;

        // Create a smooth color gradient
        // Dark blue -> cyan -> yellow -> white
        int r, g, b;

        if (t < 0.5) {
            // Dark blue to cyan
            double local_t = t * 2.0;
            r = (int)(0 + local_t * 0);
            g = (int)(64 + local_t * 128);
            b = (int)(128 + local_t * 127);
        } else {
            // Cyan to yellow to white
            double local_t = (t - 0.5) * 2.0;
            r = (int)(0 + local_t * 255);
            g = (int)(192 + local_t * 63);
            b = (int)(255 - local_t * 128);
        }

        palette[i] = RGB(r & 255, g & 255, b & 255);
    }
}

void metaballs_update(pixel_t *pixels, uint32_t time) {
    // Update metaball positions
    double time_factor = time * 0.001;

    for (int i = 0; i < NUM_BALLS; i++) {
        // Move in circular/elliptical patterns
        balls[i].x = SCREEN_WIDTH / 2 +
                     60 * sin(time_factor * balls[i].speed_x + balls[i].phase_x);
        balls[i].y = SCREEN_HEIGHT / 2 +
                     40 * cos(time_factor * balls[i].speed_y + balls[i].phase_y);
    }

    // Render each pixel
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Calculate total influence from all metaballs
            double total_influence = 0.0;

            for (int i = 0; i < NUM_BALLS; i++) {
                double dx = x - balls[i].x;
                double dy = y - balls[i].y;
                double dist_sq = dx * dx + dy * dy;

                // Avoid division by zero
                if (dist_sq < 1.0) dist_sq = 1.0;

                // Add influence (inversely proportional to distance squared)
                total_influence += (balls[i].radius * balls[i].radius) / dist_sq;
            }

            // Map influence to color intensity
            // Scale and clamp to palette range
            int intensity = (int)(total_influence * 40.0);
            if (intensity > 255) intensity = 255;
            if (intensity < 0) intensity = 0;

            // Set pixel color from palette
            pixels[y * SCREEN_WIDTH + x] = palette[intensity];
        }
    }
}

void metaballs_cleanup(void) {
    // No dynamic allocation, nothing to clean up
}
