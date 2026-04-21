#include "sdf_march.h"
#include "common.h"
#include <math.h>

#define MAX_STEPS    52
#define MIN_DIST     0.006f
#define MAX_DIST     14.0f
#define NUM_SPHERES  3
#define BLEND_K      0.55f
#define SPH_RADIUS   0.52f

typedef struct { float x, y, z; } v3;

static inline float fast_rsqrt(float x) {
    float h = 0.5f * x;
    union { float f; int i; } u;
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1);
    u.f = u.f * (1.5f - h * u.f * u.f);
    return u.f;
}

static inline v3   v3_add(v3 a, v3 b)  { return (v3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline v3   v3_sub(v3 a, v3 b)  { return (v3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline v3   v3_mul(v3 v, float s){ return (v3){v.x*s, v.y*s, v.z*s}; }
static inline float v3_dot(v3 a, v3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float v3_len(v3 v)        { float d2=v3_dot(v,v); return d2>1e-8f ? d2*fast_rsqrt(d2) : 0.0f; }
static inline v3   v3_norm(v3 v)        { float r=fast_rsqrt(fmaxf(v3_dot(v,v),1e-8f)); return v3_mul(v,r); }

static inline float smin(float a, float b, float k) {
    float h = fmaxf(k - fabsf(a - b), 0.0f) / k;
    return fminf(a, b) - h * h * k * 0.25f;
}

static v3 sph[NUM_SPHERES];

static float scene(v3 p) {
    float d = fminf(p.y + 1.5f, 1.5f - p.y);
    for (int i = 0; i < NUM_SPHERES; i++) {
        v3 dp = v3_sub(p, sph[i]);
        d = smin(d, v3_len(dp) - SPH_RADIUS, BLEND_K);
    }
    return d;
}

/* Tetrahedron normal — 4 scene evals instead of 6 */
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

void sdf_march_init(void) {}

void sdf_march_update(pixel_t *pixels, uint32_t time) {
    float t = time * 0.001f;

    /* Spheres oscillate vertically; phase-offset so they take turns touching each plane */
    sph[0] = (v3){ -0.85f, sinf(t * 1.10f)           * 0.90f, 2.2f };
    sph[1] = (v3){  0.00f, sinf(t * 0.85f + 2.094f)  * 0.90f, 3.5f };
    sph[2] = (v3){  0.85f, sinf(t * 1.30f + 4.189f)  * 0.90f, 2.8f };

    /* Camera gently bobs */
    v3 cam = { sinf(t * 0.25f) * 0.12f, sinf(t * 0.18f) * 0.08f, 0.0f };

    /* Orbiting key light */
    v3 lpos = { sinf(t * 0.55f) * 3.5f, 1.3f, sinf(t * 0.3f) * 1.5f + 2.5f };

    float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    float fov    = 0.82f;

    /* Sphere surface colours (linear, pre-gamma) */
    static const float sc[NUM_SPHERES][3] = {
        { 0.10f, 0.65f, 0.95f },   /* cyan-blue  */
        { 0.95f, 0.22f, 0.70f },   /* magenta    */
        { 0.95f, 0.60f, 0.08f },   /* amber      */
    };

    for (int py = 0; py < SCREEN_HEIGHT; py++) {
        for (int px = 0; px < SCREEN_WIDTH; px++) {
            float u =  (2.0f * px / SCREEN_WIDTH  - 1.0f) * aspect * fov;
            float v_ = (1.0f - 2.0f * py / SCREEN_HEIGHT) * fov;

            v3 dir = v3_norm((v3){u, v_, 1.0f});

            /* Ray march */
            float td = 0.1f;
            int   steps = 0;
            v3    p = cam;
            int   hit = 0;

            for (steps = 0; steps < MAX_STEPS; steps++) {
                p = v3_add(cam, v3_mul(dir, td));
                float d = scene(p);
                if (d < MIN_DIST) { hit = 1; break; }
                if (td >= MAX_DIST) break;
                td += d;
            }

            int r, g, b;

            if (hit) {
                v3 n  = calc_normal(p);
                v3 ld = v3_norm(v3_sub(lpos, p));
                v3 vd = v3_norm(v3_sub(cam, p));
                v3 hv = v3_norm(v3_add(ld, vd));

                float diff = fmaxf(v3_dot(n, ld),  0.0f);
                float spec = powf(fmaxf(v3_dot(n, hv), 0.0f), 72.0f) * 0.55f;
                float ao   = 1.0f - (float)steps / (float)MAX_STEPS * 0.65f;

                /* Material blend: weight each sphere by proximity */
                float w[NUM_SPHERES], wsum = 0.0f;
                for (int i = 0; i < NUM_SPHERES; i++) {
                    float d = fmaxf(v3_len(v3_sub(p, sph[i])) - SPH_RADIUS, 0.0f);
                    w[i] = fmaxf(0.0f, 1.0f - d / (BLEND_K * 1.8f));
                    w[i] *= w[i];
                    wsum += w[i];
                }

                /* Plane base colour — dark with neon grid on floor/ceiling */
                float pr = 0.06f, pg = 0.06f, pb = 0.10f;
                if (fabsf(n.y) > 0.45f) {
                    float gx = fmodf(fabsf(p.x) + 50.0f, 1.0f);
                    float gz = fmodf(fabsf(p.z) + 50.0f, 1.0f);
                    float grid = (gx < 0.05f || gz < 0.05f) ? 1.0f : 0.0f;
                    pr += grid * 0.04f;
                    pg += grid * 0.25f;
                    pb += grid * 0.40f;
                }

                /* Weighted sphere colour */
                float mr = pr, mg = pg, mb = pb;
                if (wsum > 0.001f) {
                    float sr = 0.0f, sg = 0.0f, sbl = 0.0f;
                    for (int i = 0; i < NUM_SPHERES; i++) {
                        sr  += sc[i][0] * w[i];
                        sg  += sc[i][1] * w[i];
                        sbl += sc[i][2] * w[i];
                    }
                    float inv = 1.0f / wsum;
                    float blend = fminf(wsum, 1.0f);
                    mr = pr + (sr*inv - pr) * blend;
                    mg = pg + (sg*inv - pg) * blend;
                    mb = pb + (sbl*inv - pb) * blend;
                }

                float lit = (0.10f + diff * 0.90f) * ao;
                float fr = fminf(mr * lit + spec, 1.0f);
                float fg = fminf(mg * lit + spec, 1.0f);
                float fb = fminf(mb * lit + spec, 1.0f);

                /* Depth fog */
                float fog = (td / MAX_DIST); fog *= fog;
                fr = fr*(1.0f-fog) + 0.01f*fog;
                fg = fg*(1.0f-fog) + 0.01f*fog;
                fb = fb*(1.0f-fog) + 0.02f*fog;

                r = (int)(fr * 255.0f);
                g = (int)(fg * 255.0f);
                b = (int)(fb * 255.0f);
            } else {
                r = 2; g = 2; b = 4;
            }

            pixels[py * SCREEN_WIDTH + px] = RGB(r, g, b);
        }
    }
}

void sdf_march_cleanup(void) {}
