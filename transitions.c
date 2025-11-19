#include "transitions.h"
#include <math.h>
#include <stdlib.h>

// Transition names for display
const char *transition_names[TRANSITION_COUNT] = {
    "Crossfade",
    "Horizontal Wipe",
    "Pixelate Dissolve",
    "Circular Wipe"
};

// Helper: blend two RGBA pixels with alpha
static inline pixel_t blend_pixels(pixel_t old_px, pixel_t new_px, float alpha) {
    // Extract RGBA components (format: [R:24-31][G:16-23][B:8-15][A:0-7])
    uint8_t old_r = (old_px >> 24) & 0xFF;
    uint8_t old_g = (old_px >> 16) & 0xFF;
    uint8_t old_b = (old_px >> 8) & 0xFF;
    uint8_t old_a = old_px & 0xFF;

    uint8_t new_r = (new_px >> 24) & 0xFF;
    uint8_t new_g = (new_px >> 16) & 0xFF;
    uint8_t new_b = (new_px >> 8) & 0xFF;
    uint8_t new_a = new_px & 0xFF;

    // Linear interpolation
    uint8_t r = (uint8_t)(old_r + alpha * (new_r - old_r));
    uint8_t g = (uint8_t)(old_g + alpha * (new_g - old_g));
    uint8_t b = (uint8_t)(old_b + alpha * (new_b - old_b));
    uint8_t a = (uint8_t)(old_a + alpha * (new_a - old_a));

    return RGBA(r, g, b, a);
}

// 1. Crossfade: smooth alpha blend
void transition_crossfade(pixel_t *dest, const pixel_t *old_frame,
                         const pixel_t *new_frame, float progress) {
    int total_pixels = SCREEN_WIDTH * SCREEN_HEIGHT;
    for (int i = 0; i < total_pixels; i++) {
        dest[i] = blend_pixels(old_frame[i], new_frame[i], progress);
    }
}

// 2. Horizontal Wipe: left-to-right reveal
void transition_horizontal_wipe(pixel_t *dest, const pixel_t *old_frame,
                               const pixel_t *new_frame, float progress) {
    int wipe_x = (int)(progress * SCREEN_WIDTH);

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            if (x < wipe_x) {
                dest[idx] = new_frame[idx];
            } else {
                dest[idx] = old_frame[idx];
            }
        }
    }
}

// 3. Pixelate Dissolve: growing block patterns
void transition_pixelate_dissolve(pixel_t *dest, const pixel_t *old_frame,
                                 const pixel_t *new_frame, float progress) {
    // Block size shrinks as transition progresses
    // Start with 32x32 blocks, end with 1x1 (full resolution)
    int max_block_size = 32;
    int block_size = (int)(max_block_size * (1.0f - progress)) + 1;

    // Use pseudo-random pattern based on block position
    for (int y = 0; y < SCREEN_HEIGHT; y += block_size) {
        for (int x = 0; x < SCREEN_WIDTH; x += block_size) {
            // Pseudo-random threshold for this block
            int block_hash = (x / block_size * 73 + y / block_size * 37) % 100;
            float threshold = block_hash / 100.0f;

            // Decide if this block shows new or old frame
            pixel_t block_pixel = (progress > threshold) ?
                                 new_frame[y * SCREEN_WIDTH + x] :
                                 old_frame[y * SCREEN_WIDTH + x];

            // Fill the block
            for (int by = 0; by < block_size && (y + by) < SCREEN_HEIGHT; by++) {
                for (int bx = 0; bx < block_size && (x + bx) < SCREEN_WIDTH; bx++) {
                    int idx = (y + by) * SCREEN_WIDTH + (x + bx);
                    dest[idx] = block_pixel;
                }
            }
        }
    }
}

// 4. Circular Wipe: expanding circle from center
void transition_circular_wipe(pixel_t *dest, const pixel_t *old_frame,
                             const pixel_t *new_frame, float progress) {
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;

    // Maximum radius to cover entire screen (diagonal distance from center)
    float max_radius = sqrtf(center_x * center_x + center_y * center_y);
    float current_radius = progress * max_radius;
    float current_radius_sq = current_radius * current_radius;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;

            // Calculate distance from center (squared to avoid sqrt)
            int dx = x - center_x;
            int dy = y - center_y;
            float dist_sq = dx * dx + dy * dy;

            // Show new frame inside circle, old frame outside
            if (dist_sq <= current_radius_sq) {
                dest[idx] = new_frame[idx];
            } else {
                dest[idx] = old_frame[idx];
            }
        }
    }
}

// Helper: apply current transition type
void transition_apply(TransitionType type, pixel_t *dest,
                     const pixel_t *old_frame, const pixel_t *new_frame,
                     float progress) {
    switch (type) {
        case TRANSITION_CROSSFADE:
            transition_crossfade(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_HORIZONTAL_WIPE:
            transition_horizontal_wipe(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_PIXELATE_DISSOLVE:
            transition_pixelate_dissolve(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_CIRCULAR_WIPE:
            transition_circular_wipe(dest, old_frame, new_frame, progress);
            break;
        default:
            // Fallback to crossfade
            transition_crossfade(dest, old_frame, new_frame, progress);
            break;
    }
}
