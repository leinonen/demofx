#include "reaction_diffusion.h"
#include <stdlib.h>

#define RD_W SCREEN_WIDTH
#define RD_H SCREEN_HEIGHT
#define RD_N (RD_W * RD_H)

static float grid_a[2][RD_N];
static float grid_b[2][RD_N];
static int   cur_buf;

static pixel_t palette[256];

typedef struct { float f, k; } RDPreset;

static const RDPreset presets[] = {
    { 0.035f, 0.065f },  // spots
    { 0.025f, 0.060f },  // stripes
    { 0.029f, 0.057f },  // mazes
    { 0.062f, 0.061f },  // coral
};
#define RD_NUM_PRESETS 4

static float cur_f, cur_k, tgt_f, tgt_k;
static int   last_preset;

static void build_palette(void) {
    for (int i = 0; i < 256; i++) {
        float t = i / 255.0f;
        int r, g, b;
        if (t < 0.33f) {
            float s = t / 0.33f;
            r = 0;
            g = (int)(s * 60);
            b = (int)(10 + s * 200);
        } else if (t < 0.66f) {
            float s = (t - 0.33f) / 0.33f;
            r = (int)(s * 180);
            g = (int)(60 + s * 200);
            b = (int)(210 - s * 210);
        } else {
            float s = (t - 0.66f) / 0.34f;
            r = (int)(180 + s * 75);
            g = 255;
            b = (int)(s * 120);
        }
        palette[i] = RGB(r, g, b);
    }
}

static void seed_grid(void) {
    for (int i = 0; i < RD_N; i++) {
        grid_a[cur_buf][i] = 1.0f;
        grid_b[cur_buf][i] = 0.0f;
    }
    for (int n = 0; n < 15; n++) {
        int cx  = 20 + rand() % (RD_W - 40);
        int cy  = 10 + rand() % (RD_H - 20);
        int rad = 2  + rand() % 6;
        for (int y = cy - rad; y <= cy + rad; y++) {
            for (int x = cx - rad; x <= cx + rad; x++) {
                if (x < 0 || x >= RD_W || y < 0 || y >= RD_H) continue;
                if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rad * rad) {
                    grid_a[cur_buf][y * RD_W + x] = 0.0f;
                    grid_b[cur_buf][y * RD_W + x] = 1.0f;
                }
            }
        }
    }
}

void reaction_diffusion_init(void) {
    build_palette();
    cur_buf = 0;
    last_preset = 0;
    cur_f = tgt_f = presets[0].f;
    cur_k = tgt_k = presets[0].k;
    seed_grid();
}

void reaction_diffusion_update(pixel_t *pixels, uint32_t time) {
    int preset = (int)((time / 12000u) % RD_NUM_PRESETS);
    if (preset != last_preset) {
        last_preset = preset;
        tgt_f = presets[preset].f;
        tgt_k = presets[preset].k;
    }
    cur_f += (tgt_f - cur_f) * 0.005f;
    cur_k += (tgt_k - cur_k) * 0.005f;

    float f = cur_f;
    float k = cur_k;

    for (int step = 0; step < 4; step++) {
        int    nxt = 1 - cur_buf;
        float *ac  = grid_a[cur_buf];
        float *bc  = grid_b[cur_buf];
        float *an  = grid_a[nxt];
        float *bn  = grid_b[nxt];

        for (int y = 0; y < RD_H; y++) {
            int ym = (y == 0)        ? RD_H - 1 : y - 1;
            int yp = (y == RD_H - 1) ? 0         : y + 1;

            for (int x = 0; x < RD_W; x++) {
                int xm = (x == 0)        ? RD_W - 1 : x - 1;
                int xp = (x == RD_W - 1) ? 0         : x + 1;

                int   ci = y * RD_W + x;
                float av = ac[ci];
                float bv = bc[ci];

                float la = 0.2f  * (ac[ym*RD_W+x]  + ac[yp*RD_W+x]  + ac[y*RD_W+xm]  + ac[y*RD_W+xp])
                         + 0.05f * (ac[ym*RD_W+xm] + ac[ym*RD_W+xp] + ac[yp*RD_W+xm] + ac[yp*RD_W+xp])
                         - av;
                float lb = 0.2f  * (bc[ym*RD_W+x]  + bc[yp*RD_W+x]  + bc[y*RD_W+xm]  + bc[y*RD_W+xp])
                         + 0.05f * (bc[ym*RD_W+xm] + bc[ym*RD_W+xp] + bc[yp*RD_W+xm] + bc[yp*RD_W+xp])
                         - bv;

                float ab2 = av * bv * bv;

                float na = av + la        - ab2 + f * (1.0f - av);
                float nb = bv + 0.5f * lb + ab2 - (k + f) * bv;

                an[ci] = na < 0.0f ? 0.0f : (na > 1.0f ? 1.0f : na);
                bn[ci] = nb < 0.0f ? 0.0f : (nb > 1.0f ? 1.0f : nb);
            }
        }
        cur_buf = nxt;
    }

    float *bv = grid_b[cur_buf];
    for (int i = 0; i < RD_N; i++) {
        int ci = (int)(bv[i] * 255.0f);
        pixels[i] = palette[ci > 255 ? 255 : ci];
    }
}

void reaction_diffusion_cleanup(void) {}
