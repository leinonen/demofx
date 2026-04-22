#include "morph3d.h"
#include "common.h"
#include <math.h>

#define MAX_STEPS  42
#define MIN_DIST   0.008f
#define MAX_DIST   10.0f

typedef struct { float x, y, z; } v3;

static inline float fast_rsqrt(float x) {
    float h = 0.5f * x;
    union { float f; int i; } u;
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1);
    u.f = u.f * (1.5f - h * u.f * u.f);
    return u.f;
}

static inline v3    v3_add(v3 a, v3 b)   { return (v3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline v3    v3_sub(v3 a, v3 b)   { return (v3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline v3    v3_mul(v3 v, float s) { return (v3){v.x*s, v.y*s, v.z*s}; }
static inline float v3_dot(v3 a, v3 b)   { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float v3_len(v3 v)          { float d2=v3_dot(v,v); return d2>1e-8f ? d2*fast_rsqrt(d2) : 0.0f; }
static inline v3    v3_norm(v3 v)         { float r=fast_rsqrt(fmaxf(v3_dot(v,v),1e-8f)); return v3_mul(v,r); }

/* Per-frame globals — set once before pixel loop, read inside scene() */
static float g_cy, g_sy, g_cx, g_sx;
static int   g_shape_a, g_shape_b;
static float g_morph;

static inline float sdf_sphere(v3 p) {
    return v3_len(p) - 0.85f;
}

static inline float sdf_rbox(v3 p) {
    float qx = fabsf(p.x) - 0.55f;
    float qy = fabsf(p.y) - 0.55f;
    float qz = fabsf(p.z) - 0.55f;
    v3 qc = { fmaxf(qx, 0.0f), fmaxf(qy, 0.0f), fmaxf(qz, 0.0f) };
    return v3_len(qc) + fminf(fmaxf(qx, fmaxf(qy, qz)), 0.0f) - 0.12f;
}

static inline float sdf_torus(v3 p) {
    float r2d = sqrtf(p.x*p.x + p.z*p.z) - 0.60f;
    return sqrtf(r2d*r2d + p.y*p.y) - 0.28f;
}

static inline float sdf_oct(v3 p) {
    return (fabsf(p.x) + fabsf(p.y) + fabsf(p.z) - 0.90f) * 0.57735f;
}

static inline float sdf_shape(v3 p, int shape) {
    switch (shape) {
        case 0: return sdf_sphere(p);
        case 1: return sdf_rbox(p);
        case 2: return sdf_torus(p);
        case 3: return sdf_oct(p);
    }
    return 1e9f;
}

static float scene(v3 p) {
    /* Rotate point into object space */
    float x1 =  g_cy*p.x + g_sy*p.z;
    float z1 = -g_sy*p.x + g_cy*p.z;
    float y2 =  g_cx*p.y - g_sx*z1;
    float z2 =  g_sx*p.y + g_cx*z1;
    v3 rp = {x1, y2, z2};

    float da = sdf_shape(rp, g_shape_a);
    float db = sdf_shape(rp, g_shape_b);
    return da + (db - da) * g_morph;
}

static v3 calc_normal(v3 p) {
    const float e = 0.003f;
    float s0 = scene((v3){p.x+e, p.y-e, p.z-e});
    float s1 = scene((v3){p.x-e, p.y-e, p.z+e});
    float s2 = scene((v3){p.x-e, p.y+e, p.z-e});
    float s3 = scene((v3){p.x+e, p.y+e, p.z+e});
    return v3_norm((v3){
        s0 - s1 - s2 + s3,
       -s0 - s1 + s2 + s3,
       -s0 + s1 - s2 + s3
    });
}

void morph3d_init(void) {}

void morph3d_update(pixel_t *pixels, uint32_t time) {
    float t = time * 0.001f;

    /* Rotation */
    float ay = t * 0.7f;
    float ax = t * 0.4f;
    g_cy = cosf(ay); g_sy = sinf(ay);
    g_cx = cosf(ax); g_sx = sinf(ax);

    /* Morph phase: 16s cycle, 4 shapes × (2.5s morph + 1.5s hold) */
    float phase   = fmodf(t * 0.0625f, 4.0f);
    g_shape_a = (int)phase % 4;
    g_shape_b = (g_shape_a + 1) % 4;
    float local_t = phase - (int)phase;

    if (local_t < 0.375f) {
        g_morph = 0.0f;
    } else if (local_t > 0.625f) {
        g_morph = 1.0f;
    } else {
        float m = (local_t - 0.375f) / 0.25f;
        g_morph = m * m * (3.0f - 2.0f * m);
    }

    int   in_morph = (g_morph > 0.05f && g_morph < 0.95f);
    float step_scale = in_morph ? 0.85f : 1.0f;

    /* Orbiting light */
    v3 lpos = { sinf(t * 0.4f) * 3.0f, 2.0f, cosf(t * 0.3f) * 1.0f - 4.0f };

    /* Surface color lerp */
    static const float shape_colors[4][3] = {
        { 0.20f, 0.70f, 0.95f },
        { 0.95f, 0.45f, 0.10f },
        { 0.85f, 0.15f, 0.75f },
        { 0.25f, 0.95f, 0.45f },
    };
    float mr = shape_colors[g_shape_a][0] + (shape_colors[g_shape_b][0] - shape_colors[g_shape_a][0]) * g_morph;
    float mg = shape_colors[g_shape_a][1] + (shape_colors[g_shape_b][1] - shape_colors[g_shape_a][1]) * g_morph;
    float mb = shape_colors[g_shape_a][2] + (shape_colors[g_shape_b][2] - shape_colors[g_shape_a][2]) * g_morph;

    float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    float fov    = 0.82f;
    v3    cam    = { 0.0f, 0.0f, -2.5f };

    for (int py = 0; py < SCREEN_HEIGHT; py++) {
        for (int px = 0; px < SCREEN_WIDTH; px++) {
            float u  =  (2.0f * px / SCREEN_WIDTH  - 1.0f) * aspect * fov;
            float vv = (1.0f - 2.0f * py / SCREEN_HEIGHT) * fov;

            v3 dir = v3_norm((v3){u, vv, 1.0f});

            float td    = 0.1f;
            int   steps = 0;
            v3    p     = cam;
            int   hit   = 0;

            for (steps = 0; steps < MAX_STEPS; steps++) {
                p = v3_add(cam, v3_mul(dir, td));
                float d = scene(p);
                if (d < MIN_DIST) { hit = 1; break; }
                if (td >= MAX_DIST) break;
                td += d * step_scale;
            }

            int r, g, b;

            if (hit) {
                v3 n  = calc_normal(p);
                v3 ld = v3_norm(v3_sub(lpos, p));
                v3 vd = v3_norm(v3_sub(cam, p));
                v3 hv = v3_norm(v3_add(ld, vd));

                float diff = fmaxf(v3_dot(n, ld), 0.0f);
                float spec = powf(fmaxf(v3_dot(n, hv), 0.0f), 64.0f) * 0.50f;
                float ao   = 1.0f - (float)steps / (float)MAX_STEPS * 0.60f;

                float lit = (0.08f + diff * 0.90f) * ao;
                float fr  = fminf(mr * lit + spec, 1.0f);
                float fg  = fminf(mg * lit + spec, 1.0f);
                float fb  = fminf(mb * lit + spec, 1.0f);

                /* Depth fog */
                float fog = td / MAX_DIST; fog *= fog;
                fr = fr*(1.0f-fog) + 0.01f*fog;
                fg = fg*(1.0f-fog) + 0.01f*fog;
                fb = fb*(1.0f-fog) + 0.02f*fog;

                r = (int)(fr * 255.0f);
                g = (int)(fg * 255.0f);
                b = (int)(fb * 255.0f);
            } else {
                r = 1; g = 1; b = 5;
            }

            pixels[py * SCREEN_WIDTH + px] = RGB(r, g, b);
        }
    }
}

void morph3d_cleanup(void) {}
