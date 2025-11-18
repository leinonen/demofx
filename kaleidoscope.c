#include "kaleidoscope.h"
#include <stdlib.h>
#include <math.h>

#define TEXTURE_SIZE 256
#define TEXTURE_MASK (TEXTURE_SIZE - 1)
#define NUM_SEGMENTS 8  // 8-fold symmetry
#define NUM_PARTICLES 32

// Source texture that will be kaleidoscoped
static pixel_t *source_texture;

// Particle for animating the source
typedef struct {
    float x, y;
    float vx, vy;
    int r, g, b;
} Particle;

static Particle particles[NUM_PARTICLES];

// Color palette for smooth gradients
static pixel_t palette[256];

// Generate color palette
static void generate_palette(void) {
    for (int i = 0; i < 256; i++) {
        float t = i / 255.0f;

        // Create rainbow-like palette with smooth transitions
        int r = (int)(128 + 127 * sinf(t * M_PI * 2.0f));
        int g = (int)(128 + 127 * sinf(t * M_PI * 2.0f + M_PI * 2.0f / 3.0f));
        int b = (int)(128 + 127 * sinf(t * M_PI * 2.0f + M_PI * 4.0f / 3.0f));

        palette[i] = RGB(r, g, b);
    }
}

// Initialize particles
static void init_particles(void) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = (rand() % TEXTURE_SIZE);
        particles[i].y = (rand() % TEXTURE_SIZE);
        particles[i].vx = (rand() % 200 - 100) / 100.0f;
        particles[i].vy = (rand() % 200 - 100) / 100.0f;

        // Random bright colors
        particles[i].r = 128 + (rand() % 128);
        particles[i].g = 128 + (rand() % 128);
        particles[i].b = 128 + (rand() % 128);
    }
}

// Update particles
static void update_particles(float dt) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;

        // Wrap around
        if (particles[i].x < 0) particles[i].x += TEXTURE_SIZE;
        if (particles[i].x >= TEXTURE_SIZE) particles[i].x -= TEXTURE_SIZE;
        if (particles[i].y < 0) particles[i].y += TEXTURE_SIZE;
        if (particles[i].y >= TEXTURE_SIZE) particles[i].y -= TEXTURE_SIZE;
    }
}

// Generate animated source texture
static void generate_source_texture(uint32_t time) {
    float t = time * 0.001f;

    // Clear texture with black
    for (int i = 0; i < TEXTURE_SIZE * TEXTURE_SIZE; i++) {
        source_texture[i] = RGB(0, 0, 0);
    }

    // Draw plasma-like background
    for (int y = 0; y < TEXTURE_SIZE; y++) {
        for (int x = 0; x < TEXTURE_SIZE; x++) {
            float fx = x / (float)TEXTURE_SIZE;
            float fy = y / (float)TEXTURE_SIZE;

            // Multi-frequency plasma
            float v = 0.0f;
            v += sinf(fx * 10.0f + t);
            v += sinf(fy * 8.0f - t * 0.7f);
            v += sinf((fx + fy) * 6.0f + t * 1.3f);
            v += sinf(sqrtf(fx * fx + fy * fy) * 12.0f - t * 2.0f);

            // Map to palette
            int idx = (int)((v + 4.0f) * 31.75f) & 0xFF;
            pixel_t color = palette[idx];

            // Dim the background
            int r = ((color >> 24) & 0xFF) / 3;
            int g = ((color >> 16) & 0xFF) / 3;
            int b = ((color >> 8) & 0xFF) / 3;

            source_texture[y * TEXTURE_SIZE + x] = RGB(r, g, b);
        }
    }

    // Draw particles with glow
    for (int i = 0; i < NUM_PARTICLES; i++) {
        int cx = (int)particles[i].x;
        int cy = (int)particles[i].y;

        // Draw particle with radial glow
        for (int dy = -16; dy <= 16; dy++) {
            for (int dx = -16; dx <= 16; dx++) {
                int px = (cx + dx) & TEXTURE_MASK;
                int py = (cy + dy) & TEXTURE_MASK;

                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 16.0f) {
                    float intensity = 1.0f - (dist / 16.0f);
                    intensity = intensity * intensity;  // Square for sharper falloff

                    pixel_t current = source_texture[py * TEXTURE_SIZE + px];
                    int cr = (current >> 24) & 0xFF;
                    int cg = (current >> 16) & 0xFF;
                    int cb = (current >> 8) & 0xFF;

                    // Add particle color
                    cr += (int)(particles[i].r * intensity);
                    cg += (int)(particles[i].g * intensity);
                    cb += (int)(particles[i].b * intensity);

                    // Clamp
                    if (cr > 255) cr = 255;
                    if (cg > 255) cg = 255;
                    if (cb > 255) cb = 255;

                    source_texture[py * TEXTURE_SIZE + px] = RGB(cr, cg, cb);
                }
            }
        }
    }
}

void kaleidoscope_init(void) {
    // Allocate source texture
    source_texture = (pixel_t*)malloc(TEXTURE_SIZE * TEXTURE_SIZE * sizeof(pixel_t));
    if (!source_texture) {
        exit(1);
    }

    // Initialize
    generate_palette();
    init_particles();
}

void kaleidoscope_update(pixel_t *pixels, uint32_t time) {
    float t = time * 0.001f;

    // Update particles
    update_particles(16.0f);  // Roughly 60fps

    // Generate source texture
    generate_source_texture(time);

    // Kaleidoscope parameters
    float rotation = t * 0.3f;
    float zoom = 1.5f + 0.5f * sinf(t * 0.5f);

    // Center point
    float cx = SCREEN_WIDTH / 2.0f;
    float cy = SCREEN_HEIGHT / 2.0f;

    // Segment angle
    float segment_angle = (2.0f * M_PI) / NUM_SEGMENTS;

    // Render kaleidoscope
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Center coordinates
            float px = x - cx;
            float py = y - cy;

            // Apply rotation
            float cos_r = cosf(rotation);
            float sin_r = sinf(rotation);
            float rx = px * cos_r - py * sin_r;
            float ry = px * sin_r + py * cos_r;

            // Calculate angle and distance from center
            float angle = atan2f(ry, rx);
            float dist = sqrtf(rx * rx + ry * ry);

            // Normalize angle to 0 to 2*PI
            if (angle < 0) angle += 2.0f * M_PI;

            // Determine which segment we're in
            float segment_idx = angle / segment_angle;
            int segment = (int)segment_idx;
            float segment_offset = segment_idx - segment;

            // Mirror alternating segments for more interesting patterns
            if (segment & 1) {
                segment_offset = 1.0f - segment_offset;
            }

            // Calculate angle within segment
            float local_angle = segment_offset * segment_angle;

            // Apply zoom
            dist *= zoom;

            // Convert back to texture coordinates
            int tx = (int)(TEXTURE_SIZE / 2 + cosf(local_angle) * dist) & TEXTURE_MASK;
            int ty = (int)(TEXTURE_SIZE / 2 + sinf(local_angle) * dist) & TEXTURE_MASK;

            // Sample from source texture
            pixels[y * SCREEN_WIDTH + x] = source_texture[ty * TEXTURE_SIZE + tx];
        }
    }
}

void kaleidoscope_cleanup(void) {
    if (source_texture) {
        free(source_texture);
        source_texture = NULL;
    }
}
