#include "spiky_twister.h"
#include <math.h>

#define N_THETA    2048
#define N_A        7        /* spike columns around circumference */
#define P_A        3        /* angular profile sharpness (softer) */
#define P_V        5        /* vertical profile sharpness */
#define R_BASE     48.0
#define SPIKE_AMP  13.0

/* Per-column organic variation */
static const double col_amp[N_A]    = { 1.0,  0.55, 0.85, 0.45, 0.95, 0.65, 0.50 };
static const double col_voff[N_A]   = { 0.0,  2.7,  1.3,  4.8,  0.6,  3.9,  5.5  };
static const double col_vscale[N_A] = { 1.0,  1.15, 0.80, 1.30, 0.90, 1.10, 0.70 };
/* Angular offsets break the even 2π/7 spacing */
static const double col_aoff[N_A]   = { 0.0,  0.18, -0.22, 0.31, -0.14, 0.25, -0.29 };

#define V_FREQ_BASE  (8.0 * M_PI / SCREEN_HEIGHT)

void spiky_twister_init(void) {}
void spiky_twister_cleanup(void) {}

static double fast_pow32(double x) {
    double x2 = x*x, x4 = x2*x2, x8 = x4*x4, x16 = x8*x8;
    return x16 * x16;
}

void spiky_twister_update(pixel_t *pixels, uint32_t time) {
    const double t = time * 0.001;

    const double spike_rot = t * 1.8;
    const double v_phase   = t * 0.5;

    static float zbuf[SCREEN_WIDTH];
    static float tbuf[SCREEN_WIDTH];

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
        pixels[i] = RGB(2, 0, 8);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++)
            zbuf[x] = -1e9f;

        const double cx = SCREEN_WIDTH / 2.0
                        + sin(y * 0.025 + t * 1.1) * 55.0
                        + sin(y * 0.013 - t * 0.7) * 30.0;

        /* Precompute per-column vertical factors for this scanline */
        double gv[N_A], dgv[N_A];
        for (int i = 0; i < N_A; i++) {
            const double phase = col_vscale[i] * V_FREQ_BASE * y + col_voff[i] + v_phase;
            const double cv = cos(phase);
            const double sv = sin(phase);
            if (cv > 0.0) {
                const double cv2 = cv * cv;
                gv[i]  = cv2 * cv2 * cv;                                          /* ^5 */
                dgv[i] = -col_vscale[i] * V_FREQ_BASE * 5.0 * cv2 * cv2 * sv;   /* d/dy */
            } else {
                gv[i] = dgv[i] = 0.0;
            }
        }

        /* Rasterize spiked profile into scanline z-buffer */
        for (int j = 0; j < N_THETA; j++) {
            const double theta = 2.0 * M_PI * j / N_THETA;
            double r = R_BASE;
            for (int i = 0; i < N_A; i++) {
                if (gv[i] == 0.0) continue;
                const double cv_a = cos(N_A * (theta - (i * 2.0*M_PI/N_A) - col_aoff[i] - spike_rot));
                if (cv_a <= 0.0) continue;
                const double cv_a2 = cv_a * cv_a;
                r += SPIKE_AMP * col_amp[i] * cv_a2 * cv_a * gv[i];              /* f^3 * g^5 */
            }

            const int px = (int)(cx + r * sin(theta) + 0.5);
            if ((unsigned)px >= (unsigned)SCREEN_WIDTH) continue;

            const float sz = (float)(r * cos(theta));
            if (sz > zbuf[px]) {
                zbuf[px] = sz;
                tbuf[px] = (float)theta;
            }
        }

        /* Shading pass */
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (zbuf[x] < -1e8f) continue;

            const double theta = tbuf[x];
            double r = R_BASE, dr_t = 0.0, dr_y = 0.0;

            for (int i = 0; i < N_A; i++) {
                const double theta_i = i * 2.0 * M_PI / N_A + col_aoff[i];
                const double cv_a = cos(N_A * (theta - theta_i - spike_rot));
                const double sv_a = sin(N_A * (theta - theta_i - spike_rot));
                if (cv_a <= 0.0) continue;
                const double cv_a2 = cv_a * cv_a;
                const double f_a   = cv_a2 * cv_a;                           /* ^3 */
                const double df_a  = -N_A * 3.0 * cv_a2 * sv_a;
                const double w     = SPIKE_AMP * col_amp[i];
                r    += w * f_a  * gv[i];
                dr_t += w * df_a * gv[i];
                dr_y += w * f_a  * dgv[i];
            }

            /* 3D surface normal: cross(dP/dtheta, dP/dy) */
            double nx =  r * sin(theta) - dr_t * cos(theta);
            double ny = -r * dr_y;
            double nz =  dr_t * sin(theta) + r * cos(theta);
            const double inv_n = 1.0 / sqrt(nx*nx + ny*ny + nz*nz);
            nx *= inv_n; ny *= inv_n; nz *= inv_n;

            /* Key light: cool white, upper-left-front */
            const double lx = -0.392, ly = -0.588, lz = 0.707;
            const double NdotL = nx*lx + ny*ly + nz*lz;
            const double diff  = NdotL > 0.0 ? NdotL : 0.0;

            const double Rz   = 2.0 * NdotL * nz - lz;
            const double spec = fast_pow32(Rz > 0.0 ? Rz : 0.0) * 1.4;

            /* Rim light from behind */
            const double rim_v = -nz * 0.7 + nx * 0.25;
            const double rim   = rim_v > 0.0 ? rim_v * rim_v * 0.75 : 0.0;

            const double amb = 0.06;
            double rc = amb + diff * 0.22 + spec       + rim * 0.95;
            double gc = amb + diff * 0.32 + spec * 0.9 + rim * 0.38;
            double bc = amb + diff * 0.90 + spec       + rim * 0.05;

#define CLAMP255(v) ((int)((v) < 0.0 ? 0 : (v) > 1.0 ? 255 : (v) * 255.0))
            pixels[y * SCREEN_WIDTH + x] = RGB(CLAMP255(rc), CLAMP255(gc), CLAMP255(bc));
        }
    }
}
