#include "bumpmap.h"
#include <stdlib.h>
#include <math.h>

#define TEXTURE_SIZE 256
#define TEXTURE_MASK (TEXTURE_SIZE - 1)
#define NUM_LIGHTS 3

// Height map for bump mapping
static unsigned char *height_map;

// Base texture
static pixel_t *base_texture;

// Light structure
typedef struct {
    float x, y, z;
    int r, g, b;
} Light;

static Light lights[NUM_LIGHTS];

// Helper function to clamp values
static inline int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Generate procedural height map with animation
static void generate_height_map(uint32_t time) {
    float t = time * 0.001f;

    // Brick dimensions
    int brick_width = 32;
    int brick_height = 16;
    int mortar = 2;

    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            // Determine brick row
            int row = y / brick_height;

            // Offset every other row for brick pattern
            int x_offset = (row & 1) ? brick_width / 2 : 0;
            int x_adjusted = (x + x_offset) % TEXTURE_SIZE;

            // Position within brick grid
            int brick_x = x_adjusted % brick_width;
            int brick_y = y % brick_height;

            // Determine if we're in mortar or brick
            int is_mortar = (brick_x < mortar || brick_y < mortar);

            // Base height
            float height = is_mortar ? 0.1f : 0.9f;

            // Add slight variation to brick surface with subtle animation
            if (!is_mortar) {
                float fx = x / 64.0f;
                float fy = y / 64.0f;
                float variation = sinf(fx + t * 0.3f) * cosf(fy - t * 0.2f) * 0.05f;
                height += variation;

                // Add slight beveling to brick edges
                float edge_dist_x = fminf(brick_x - mortar, brick_width - brick_x - 1);
                float edge_dist_y = fminf(brick_y - mortar, brick_height - brick_y - 1);
                float edge_dist = fminf(edge_dist_x, edge_dist_y);

                if (edge_dist < 2) {
                    height -= (2.0f - edge_dist) * 0.1f;
                }
            }

            // Normalize to 0-255
            int h = (int)(height * 255.0f);
            height_map[y * TEXTURE_SIZE + x] = clamp(h, 0, 255);
        }
    }
}

// Generate base texture
static void generate_base_texture(void) {
    // Brick dimensions (must match height map)
    int brick_width = 32;
    int brick_height = 16;
    int mortar = 2;

    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            // Determine brick row
            int row = y / brick_height;

            // Offset every other row for brick pattern
            int x_offset = (row & 1) ? brick_width / 2 : 0;
            int x_adjusted = (x + x_offset) % TEXTURE_SIZE;

            // Position within brick grid
            int brick_x = x_adjusted % brick_width;
            int brick_y = y % brick_height;

            // Determine if we're in mortar or brick
            int is_mortar = (brick_x < mortar || brick_y < mortar);

            int r, g, b;
            if (is_mortar) {
                // Gray mortar color
                r = 90;
                g = 90;
                b = 90;
            } else {
                // Reddish-orange brick color with variation per brick
                int brick_num = (x_adjusted / brick_width) + row * 100;
                int variation = (brick_num * 37) % 40 - 20;

                r = 160 + variation;
                g = 80 + variation / 2;
                b = 60 + variation / 3;

                // Add subtle texture within brick
                int tex_var = ((brick_x * 7) + (brick_y * 13)) % 20 - 10;
                r += tex_var;
                g += tex_var;
                b += tex_var;
            }

            base_texture[y * TEXTURE_SIZE + x] = RGB(
                clamp(r, 0, 255),
                clamp(g, 0, 255),
                clamp(b, 0, 255)
            );
        }
    }
}

void bumpmap_init(void) {
    // Allocate height map
    height_map = (unsigned char*)malloc(TEXTURE_SIZE * TEXTURE_SIZE);
    if (!height_map) {
        exit(1);
    }

    // Allocate base texture
    base_texture = (pixel_t*)malloc(TEXTURE_SIZE * TEXTURE_SIZE * sizeof(pixel_t));
    if (!base_texture) {
        free(height_map);
        exit(1);
    }

    // Generate base texture
    generate_base_texture();

    // Initialize lights with different colors
    lights[0].r = 255; lights[0].g = 50;  lights[0].b = 50;  // Red
    lights[1].r = 50;  lights[1].g = 50;  lights[1].b = 255; // Blue
    lights[2].r = 255; lights[2].g = 255; lights[2].b = 50;  // Yellow

    // Set initial z position for all lights
    for (int i = 0; i < NUM_LIGHTS; i++) {
        lights[i].z = 50.0f;
    }
}

void bumpmap_update(pixel_t *pixels, uint32_t time) {
    float t = time * 0.001f;

    // Generate animated height map
    generate_height_map(time);

    // Update light positions
    lights[0].x = SCREEN_WIDTH / 2.0f + cosf(t * 1.2f) * SCREEN_WIDTH * 0.4f;
    lights[0].y = SCREEN_HEIGHT / 2.0f + sinf(t * 1.2f) * SCREEN_HEIGHT * 0.4f;

    lights[1].x = SCREEN_WIDTH / 2.0f + cosf(t * 0.8f + 2.0f) * SCREEN_WIDTH * 0.35f;
    lights[1].y = SCREEN_HEIGHT / 2.0f + sinf(t * 1.5f + 2.0f) * SCREEN_HEIGHT * 0.35f;

    lights[2].x = SCREEN_WIDTH / 2.0f + cosf(t * 1.5f + 4.0f) * SCREEN_WIDTH * 0.3f;
    lights[2].y = SCREEN_HEIGHT / 2.0f + sinf(t * 0.9f + 4.0f) * SCREEN_HEIGHT * 0.3f;

    // Render the bump mapped surface
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Map screen coordinates to texture coordinates
            int tx = (x * TEXTURE_SIZE) / SCREEN_WIDTH;
            int ty = (y * TEXTURE_SIZE) / SCREEN_HEIGHT;

            // Sample height map and neighbors for normal calculation
            int tx_left = (tx - 1) & TEXTURE_MASK;
            int tx_right = (tx + 1) & TEXTURE_MASK;
            int ty_up = (ty - 1) & TEXTURE_MASK;
            int ty_down = (ty + 1) & TEXTURE_MASK;

            float h_left = height_map[ty * TEXTURE_SIZE + tx_left];
            float h_right = height_map[ty * TEXTURE_SIZE + tx_right];
            float h_up = height_map[ty_up * TEXTURE_SIZE + tx];
            float h_down = height_map[ty_down * TEXTURE_SIZE + tx];

            // Calculate height gradients with bump scale to exaggerate bumps
            float bump_scale = 5.0f;
            float dx = (h_right - h_left) * bump_scale / 255.0f;
            float dy = (h_down - h_up) * bump_scale / 255.0f;
            float dz = 1.0f;

            // Normalize the normal vector
            float len = sqrtf(dx * dx + dy * dy + dz * dz);
            dx /= len;
            dy /= len;
            dz /= len;

            // Get base color from texture
            pixel_t base_color = base_texture[ty * TEXTURE_SIZE + tx];
            int base_r = (base_color >> 24) & 0xFF;
            int base_g = (base_color >> 16) & 0xFF;
            int base_b = (base_color >> 8) & 0xFF;

            // Accumulate lighting from all lights
            float total_r = 0.0f, total_g = 0.0f, total_b = 0.0f;

            for (int i = 0; i < NUM_LIGHTS; i++) {
                // Calculate light direction
                float lx = lights[i].x - x;
                float ly = lights[i].y - y;
                float lz = lights[i].z;

                // Normalize light direction
                float dist = sqrtf(lx * lx + ly * ly + lz * lz);
                if (dist < 0.1f) dist = 0.1f;

                lx /= dist;
                ly /= dist;
                lz /= dist;

                // Calculate dot product (diffuse lighting)
                float intensity = dx * lx + dy * ly + dz * lz;
                if (intensity < 0.0f) intensity = 0.0f;

                // Add distance attenuation
                float attenuation = 1.0f / (1.0f + dist * 0.01f);
                intensity *= attenuation;

                // Accumulate light contribution
                total_r += intensity * lights[i].r;
                total_g += intensity * lights[i].g;
                total_b += intensity * lights[i].b;
            }

            // Add ambient light (only once, not per light)
            float ambient = 0.05f;
            total_r += ambient * 255.0f;
            total_g += ambient * 255.0f;
            total_b += ambient * 255.0f;

            // Apply lighting to base color
            int final_r = clamp((int)(base_r * total_r / 255.0f), 0, 255);
            int final_g = clamp((int)(base_g * total_g / 255.0f), 0, 255);
            int final_b = clamp((int)(base_b * total_b / 255.0f), 0, 255);

            pixels[y * SCREEN_WIDTH + x] = RGB(final_r, final_g, final_b);
        }
    }
}

void bumpmap_cleanup(void) {
    if (height_map) {
        free(height_map);
        height_map = NULL;
    }
    if (base_texture) {
        free(base_texture);
        base_texture = NULL;
    }
}
