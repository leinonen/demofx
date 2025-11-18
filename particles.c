#include "particles.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_PARTICLES 2000
#define EXPLOSION_INTERVAL 60  // frames between explosions (~1 second at 60fps)
#define PARTICLES_PER_EXPLOSION 80
#define GRAVITY 0.15f
#define VELOCITY_DECAY 0.98f
#define BOUNCE_DAMPING 0.7f
#define PARTICLE_SIZE 2

typedef struct {
    float x, y;          // position
    float vx, vy;        // velocity
    float age;           // current age in frames
    float lifetime;      // total lifetime in frames
    int active;          // is particle active
} Particle;

static Particle particles[MAX_PARTICLES];
static int frame_counter = 0;

// Get fire palette color based on particle age (0.0 = young, 1.0 = old)
static pixel_t get_fire_color(float age_ratio) {
    int r, g, b;

    if (age_ratio < 0.2f) {
        // Young: White to yellow
        float t = age_ratio / 0.2f;
        r = 255;
        g = 255;
        b = (int)(255 * (1.0f - t));
    } else if (age_ratio < 0.5f) {
        // Middle: Yellow to orange
        float t = (age_ratio - 0.2f) / 0.3f;
        r = 255;
        g = (int)(255 - 100 * t);  // 255 -> 155
        b = 0;
    } else if (age_ratio < 0.8f) {
        // Older: Orange to red
        float t = (age_ratio - 0.5f) / 0.3f;
        r = 255;
        g = (int)(155 * (1.0f - t));  // 155 -> 0
        b = 0;
    } else {
        // Old: Red to dark red to black
        float t = (age_ratio - 0.8f) / 0.2f;
        r = (int)(255 * (1.0f - t));
        g = 0;
        b = 0;
    }

    return RGB(r, g, b);
}

// Spawn an explosion at given position
static void spawn_explosion(float x, float y) {
    int spawned = 0;

    for (int i = 0; i < MAX_PARTICLES && spawned < PARTICLES_PER_EXPLOSION; i++) {
        if (!particles[i].active) {
            // Random angle and speed
            float angle = (rand() % 360) * M_PI / 180.0f;
            float speed = 1.0f + (rand() % 100) / 20.0f;  // 1.0 to 6.0

            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = cosf(angle) * speed;
            particles[i].vy = sinf(angle) * speed;
            particles[i].age = 0;
            particles[i].lifetime = 120 + (rand() % 120);  // 2-4 seconds at 60fps
            particles[i].active = 1;
            spawned++;
        }
    }
}

void particles_init(void) {
    // Initialize all particles as inactive
    memset(particles, 0, sizeof(particles));
    frame_counter = 0;

    // Seed random number generator
    srand(12345);
}

void particles_update(pixel_t *pixels, uint32_t time) {
    // Apply blur/fade effect instead of clearing screen
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixel_t color = pixels[i];
        int r = (color >> 24) & 0xFF;
        int g = (color >> 16) & 0xFF;
        int b = (color >> 8) & 0xFF;

        // Darken each component (creates motion blur trail)
        r = (r * 9) / 10;
        g = (g * 9) / 10;
        b = (b * 9) / 10;

        pixels[i] = RGB(r, g, b);
    }

    // Spawn new explosions periodically
    frame_counter++;
    if (frame_counter >= EXPLOSION_INTERVAL) {
        float x = 50 + (rand() % (SCREEN_WIDTH - 100));
        float y = 50 + (rand() % (SCREEN_HEIGHT - 100));
        spawn_explosion(x, y);
        frame_counter = 0;
    }

    // Update and draw all particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        Particle *p = &particles[i];

        // Update age
        p->age += 1.0f;
        if (p->age >= p->lifetime) {
            p->active = 0;
            continue;
        }

        // Apply gravity
        p->vy += GRAVITY;

        // Apply velocity decay (air resistance)
        p->vx *= VELOCITY_DECAY;
        p->vy *= VELOCITY_DECAY;

        // Update position
        p->x += p->vx;
        p->y += p->vy;

        // Bounce off edges with energy loss
        if (p->x < 0) {
            p->x = 0;
            p->vx = -p->vx * BOUNCE_DAMPING;
        } else if (p->x >= SCREEN_WIDTH) {
            p->x = SCREEN_WIDTH - 1;
            p->vx = -p->vx * BOUNCE_DAMPING;
        }

        if (p->y < 0) {
            p->y = 0;
            p->vy = -p->vy * BOUNCE_DAMPING;
        } else if (p->y >= SCREEN_HEIGHT) {
            p->y = SCREEN_HEIGHT - 1;
            p->vy = -p->vy * BOUNCE_DAMPING;
        }

        // Get color based on age
        float age_ratio = p->age / p->lifetime;
        pixel_t color = get_fire_color(age_ratio);

        // Draw particle (2x2 pixels for better visibility)
        int px = (int)p->x;
        int py = (int)p->y;

        for (int dy = 0; dy < PARTICLE_SIZE; dy++) {
            for (int dx = 0; dx < PARTICLE_SIZE; dx++) {
                int x = px + dx;
                int y = py + dy;
                if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
                    pixels[y * SCREEN_WIDTH + x] = color;
                }
            }
        }
    }
}

void particles_cleanup(void) {
    // Nothing to clean up (using static arrays)
}
