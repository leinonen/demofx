#include "voronoi.h"
#include <math.h>

#define NUM_SEEDS 14

typedef struct {
    float x, y;
    float vx, vy;   // Lissajous frequencies
    float px, py;   // Phase offsets
} Seed;

static Seed seeds[NUM_SEEDS];
static pixel_t palette[NUM_SEEDS];

static float lissajous_freqs[NUM_SEEDS][4] = {
    {1.0f, 2.0f, 0.0f, 1.1f},
    {2.0f, 3.0f, 0.5f, 0.7f},
    {1.5f, 1.0f, 1.2f, 0.3f},
    {3.0f, 2.0f, 0.8f, 1.6f},
    {1.0f, 3.0f, 2.1f, 0.9f},
    {2.5f, 1.5f, 1.7f, 2.3f},
    {1.3f, 2.7f, 0.4f, 1.8f},
    {3.0f, 1.0f, 1.5f, 0.6f},
    {2.0f, 1.5f, 2.8f, 1.3f},
    {1.7f, 3.0f, 0.2f, 2.1f},
    {2.3f, 2.0f, 1.1f, 0.4f},
    {1.0f, 1.7f, 2.4f, 1.9f},
    {3.0f, 2.5f, 0.7f, 2.6f},
    {2.0f, 3.0f, 1.9f, 0.1f},
};

void voronoi_init(void) {
    for (int i = 0; i < NUM_SEEDS; i++) {
        seeds[i].px = lissajous_freqs[i][2];
        seeds[i].py = lissajous_freqs[i][3];
        seeds[i].vx = lissajous_freqs[i][0];
        seeds[i].vy = lissajous_freqs[i][1];

        // Spread initial positions
        seeds[i].x = (float)(SCREEN_WIDTH / 2);
        seeds[i].y = (float)(SCREEN_HEIGHT / 2);

        // HSV rainbow palette — evenly spaced hues, full saturation
        float hue = (float)i / NUM_SEEDS;
        float h6 = hue * 6.0f;
        int sector = (int)h6;
        float frac = h6 - sector;
        float q = 1.0f - frac;
        float r, g, b;
        switch (sector % 6) {
            case 0: r = 1.0f; g = frac; b = 0.0f; break;
            case 1: r = q;    g = 1.0f; b = 0.0f; break;
            case 2: r = 0.0f; g = 1.0f; b = frac; break;
            case 3: r = 0.0f; g = q;    b = 1.0f; break;
            case 4: r = frac; g = 0.0f; b = 1.0f; break;
            default: r = 1.0f; g = 0.0f; b = q;   break;
        }
        palette[i] = RGB((int)(r * 220), (int)(g * 220), (int)(b * 220));
    }
}

void voronoi_update(pixel_t *pixels, uint32_t time) {
    float t = time * 0.0005f;

    // Move seeds along Lissajous paths
    int sx[NUM_SEEDS], sy[NUM_SEEDS];
    float margin_x = SCREEN_WIDTH * 0.35f;
    float margin_y = SCREEN_HEIGHT * 0.35f;
    float cx = SCREEN_WIDTH * 0.5f;
    float cy = SCREEN_HEIGHT * 0.5f;

    for (int i = 0; i < NUM_SEEDS; i++) {
        sx[i] = (int)(cx + margin_x * sinf(seeds[i].vx * t + seeds[i].px));
        sy[i] = (int)(cy + margin_y * sinf(seeds[i].vy * t + seeds[i].py));
    }

    // Color palette offset cycles over time for hue rotation
    float palette_shift = t * 0.15f;
    int shift = (int)(palette_shift * NUM_SEEDS) % NUM_SEEDS;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int nearest = 0, second = 0;
            int d1 = 0x7fffffff, d2 = 0x7fffffff;

            for (int i = 0; i < NUM_SEEDS; i++) {
                int dx = x - sx[i];
                int dy = y - sy[i];
                int d = dx * dx + dy * dy;
                if (d < d1) {
                    d2 = d1; second = nearest;
                    d1 = d;  nearest = i;
                } else if (d < d2) {
                    d2 = d;  second = i;
                }
            }
            (void)second;

            // Edge proximity: how close to the cell boundary
            int edge_dist = d2 - d1;
            int idx = (nearest + shift) % NUM_SEEDS;
            pixel_t base = palette[idx];

            // Extract channels and darken near edges (stained-glass look)
            uint8_t r = (base >> 24) & 0xff;
            uint8_t g = (base >> 16) & 0xff;
            uint8_t b = (base >>  8) & 0xff;

            if (edge_dist < 400) {
                // Scale brightness by edge proximity
                int scale = edge_dist * 255 / 400;
                r = (uint8_t)((r * scale) >> 8);
                g = (uint8_t)((g * scale) >> 8);
                b = (uint8_t)((b * scale) >> 8);
            }

            pixels[y * SCREEN_WIDTH + x] = RGB(r, g, b);
        }
    }

}

void voronoi_cleanup(void) {
}
