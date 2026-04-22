#include "parallax.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_LAYERS 4
#define PROFILE_WIDTH 640

static int profiles[NUM_LAYERS][PROFILE_WIDTH];

// Pixels per millisecond; ratio 1:2:4:8 between layers
static const float speeds[NUM_LAYERS] = { 0.018f, 0.036f, 0.072f, 0.144f };

static const pixel_t layer_colors[NUM_LAYERS] = {
    RGB(45, 15, 65),   // distant mountains: deep purple
    RGB(25, 12, 48),   // mid mountains: dark purple-blue
    RGB(15, 22, 35),   // rolling hills: dark blue-grey
    RGB(6,  20,  6),   // foreground trees: near-black green
};

static void gen_terrain(int layer, int *profile) {
    float base  = SCREEN_HEIGHT * (0.42f + layer * 0.09f);
    float amp   = SCREEN_HEIGHT * (0.14f - layer * 0.015f);
    float phase = layer * 137.5f;

    for (int x = 0; x < PROFILE_WIDTH; x++) {
        float h = base
            + amp * 0.50f * sinf((x + phase)          * 0.022f)
            + amp * 0.28f * sinf((x + phase * 1.4f)   * 0.057f + 1.5f)
            + amp * 0.14f * sinf((x + phase * 0.6f)   * 0.121f + 0.8f)
            + amp * 0.08f * sinf((x + phase * 2.2f)   * 0.263f + 2.1f);
        profile[x] = (int)h;
    }
}

static void gen_trees(int *profile) {
    // Start with slightly bumpy ground
    float base = SCREEN_HEIGHT * 0.72f;
    for (int x = 0; x < PROFILE_WIDTH; x++) {
        profile[x] = (int)(base
            + 4.0f * sinf(x * 0.08f + 0.3f)
            + 2.0f * sinf(x * 0.17f + 1.1f));
    }

    // Place triangle trees along the profile
    int x = 2;
    while (x < PROFILE_WIDTH - 2) {
        int tree_h = 14 + (rand() % 12);
        int tree_w =  5 + (rand() % 5);
        int ground  = profile[x];
        for (int tx = -tree_w; tx <= tree_w; tx++) {
            int idx = (x + tx + PROFILE_WIDTH) % PROFILE_WIDTH;
            int h   = tree_h - abs(tx) * tree_h / tree_w;
            if (profile[idx] > ground - h)
                profile[idx] = ground - h;
        }
        x += tree_w * 2 + (rand() % 12) + 4;
    }
}

void parallax_init(void) {
    for (int i = 0; i < NUM_LAYERS - 1; i++)
        gen_terrain(i, profiles[i]);
    gen_trees(profiles[NUM_LAYERS - 1]);
}

void parallax_update(pixel_t *pixels, uint32_t time) {
    // Sky gradient: dark blue-black at top → dusky purple at horizon
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float t = (float)y / SCREEN_HEIGHT;
        pixel_t sky = RGB(
            (int)(t * 55),
            (int)(t *  8),
            (int)(25 + t * 35)
        );
        for (int x = 0; x < SCREEN_WIDTH; x++)
            pixels[y * SCREEN_WIDTH + x] = sky;
    }

    // Stars — sparse dots in upper half, with gentle twinkle
    for (int y = 0; y < SCREEN_HEIGHT / 2; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            unsigned int h = (unsigned int)(x * 7919u + y * 6271u);
            if ((h % 280) == 0) {
                int base_b = 140 + (int)(h % 90);
                int twinkle = (int)(12.0f * sinf(time * 0.003f + (h & 0xff) * 0.09f));
                int b = base_b + twinkle;
                if (b < 0)   b = 0;
                if (b > 255) b = 255;
                pixels[y * SCREEN_WIDTH + x] = RGB(b, b, b);
            }
        }
    }

    // Moon: small circle near upper-right
    {
        int mx = SCREEN_WIDTH - 55, my = 30, mr = 10;
        for (int dy = -mr; dy <= mr; dy++) {
            for (int dx = -mr; dx <= mr; dx++) {
                if (dx * dx + dy * dy <= mr * mr) {
                    int px = mx + dx, py = my + dy;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                        pixels[py * SCREEN_WIDTH + px] = RGB(200, 190, 160);
                }
            }
        }
    }

    // Draw layers back to front
    for (int layer = 0; layer < NUM_LAYERS; layer++) {
        int offset = (int)(time * speeds[layer]) % PROFILE_WIDTH;

        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int px = (x + offset) % PROFILE_WIDTH;
            int horizon = profiles[layer][px];
            if (horizon < 0) horizon = 0;
            if (horizon >= SCREEN_HEIGHT) continue;

            pixel_t col = layer_colors[layer];
            int base = horizon * SCREEN_WIDTH;
            for (int y = horizon; y < SCREEN_HEIGHT; y++) {
                pixels[base + x] = col;
                base += SCREEN_WIDTH;
            }
        }
    }
}

void parallax_cleanup(void) {
    // nothing allocated dynamically
}
