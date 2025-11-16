#include "vectorballs.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Number of dots on the sphere
#define NUM_DOTS 600

// Sphere vertex structure
typedef struct {
    float x, y, z;
} vec3_t;

// Original sphere vertices
static vec3_t sphere_vertices[NUM_DOTS];

void vectorballs_init(void) {
    // Generate sphere vertices using spherical coordinates
    int idx = 0;
    float radius = 60.0;

    // Fibonacci sphere distribution for even dot placement
    float phi = (1.0 + sqrt(5.0)) / 2.0;  // Golden ratio
    float golden_angle = 2.0 * M_PI / (phi * phi);

    for (int i = 0; i < NUM_DOTS; i++) {
        float t = (float)i / (float)NUM_DOTS;
        float inclination = acos(1.0 - 2.0 * t);
        float azimuth = golden_angle * i;

        sphere_vertices[idx].x = radius * sin(inclination) * cos(azimuth);
        sphere_vertices[idx].y = radius * sin(inclination) * sin(azimuth);
        sphere_vertices[idx].z = radius * cos(inclination);
        idx++;
    }
}

void vectorballs_update(pixel_t *pixels, uint32_t time) {
    // Clear screen to black
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Rotation angles
    float angle_x = time * 0.0008;
    float angle_y = time * 0.001;
    float angle_z = time * 0.0006;

    // Precompute sin/cos for rotations
    float sx = sin(angle_x);
    float cx = cos(angle_x);
    float sy = sin(angle_y);
    float cy = cos(angle_y);
    float sz = sin(angle_z);
    float cz = cos(angle_z);

    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;
    float distance = 256.0;

    // Transform and render all dots
    for (int i = 0; i < NUM_DOTS; i++) {
        vec3_t v = sphere_vertices[i];

        // Rotate around X axis
        float y1 = v.y * cx - v.z * sx;
        float z1 = v.y * sx + v.z * cx;
        float x1 = v.x;

        // Rotate around Y axis
        float x2 = x1 * cy - z1 * sy;
        float z2 = x1 * sy + z1 * cy;
        float y2 = y1;

        // Rotate around Z axis
        float x3 = x2 * cz - y2 * sz;
        float y3 = x2 * sz + y2 * cz;
        float z3 = z2;

        // Add some offset for depth
        z3 += distance;

        // Skip if behind camera
        if (z3 <= 0) continue;

        // Perspective projection
        float factor = distance / z3;
        int screen_x = (int)(x3 * factor) + center_x;
        int screen_y = (int)(y3 * factor) + center_y;

        // Bounds checking
        if (screen_x < 0 || screen_x >= SCREEN_WIDTH ||
            screen_y < 0 || screen_y >= SCREEN_HEIGHT) {
            continue;
        }

        // Calculate color based on depth (closer = brighter)
        int brightness = (int)(z3 - 150);
        if (brightness < 0) brightness = 0;
        if (brightness > 255) brightness = 255;

        // Color gradient based on position
        int color_index = (int)((angle_y * 50.0) + (z3 * 0.5)) & 255;
        int r = (sine_table[color_index] + 128) * brightness / 255;
        int g = (sine_table[(color_index + 85) & 255] + 128) * brightness / 255;
        int b = (sine_table[(color_index + 170) & 255] + 128) * brightness / 255;

        // Draw the dot with size based on depth
        int dot_size = (brightness > 180) ? 2 : 1;

        for (int dy = 0; dy < dot_size; dy++) {
            for (int dx = 0; dx < dot_size; dx++) {
                int px = screen_x + dx;
                int py = screen_y + dy;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    pixels[py * SCREEN_WIDTH + px] = RGB(r, g, b);
                }
            }
        }
    }
}

void vectorballs_cleanup(void) {
    // Nothing to clean up
}
