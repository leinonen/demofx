#include "transitions.h"
#include <math.h>
#include <stdlib.h>

// Transition names for display
const char *transition_names[TRANSITION_COUNT] = {
    "Crossfade",
    "Horizontal Wipe",
    "Pixelate Dissolve",
    "Circular Wipe",
    "Flash",
    "Checkerboard",
    "Scanline",
    "Slide",
    "Diagonal Wipe",
    "Zoom"
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

// 5. Flash: fade to white then reveal new frame
void transition_flash(pixel_t *dest, const pixel_t *old_frame,
                     const pixel_t *new_frame, float progress) {
    pixel_t white = RGB(255, 255, 255);
    int total_pixels = SCREEN_WIDTH * SCREEN_HEIGHT;
    if (progress < 0.5f) {
        float a = progress * 2.0f;
        for (int i = 0; i < total_pixels; i++)
            dest[i] = blend_pixels(old_frame[i], white, a);
    } else {
        float a = (progress - 0.5f) * 2.0f;
        for (int i = 0; i < total_pixels; i++)
            dest[i] = blend_pixels(white, new_frame[i], a);
    }
}

// 6. Checkerboard: 8x8 blocks reveal in pseudo-random order
void transition_checkerboard(pixel_t *dest, const pixel_t *old_frame,
                             const pixel_t *new_frame, float progress) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        int by = y / 8;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int bx = x / 8;
            float threshold = ((bx * 3 + by * 7) % 20) / 20.0f;
            int idx = y * SCREEN_WIDTH + x;
            dest[idx] = (progress >= threshold) ? new_frame[idx] : old_frame[idx];
        }
    }
}

// 7. Scanline: even lines reveal first, odd lines second
void transition_scanline(pixel_t *dest, const pixel_t *old_frame,
                         const pixel_t *new_frame, float progress) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float local = (y % 2 == 0) ? fminf(progress * 2.0f, 1.0f)
                                   : fmaxf((progress - 0.5f) * 2.0f, 0.0f);
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            dest[idx] = blend_pixels(old_frame[idx], new_frame[idx], local);
        }
    }
}

// 8. Slide: old frame exits left, new frame enters from right
void transition_slide(pixel_t *dest, const pixel_t *old_frame,
                      const pixel_t *new_frame, float progress) {
    int split_x = (int)(progress * SCREEN_WIDTH);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            if (x < split_x) {
                int src_x = x + (SCREEN_WIDTH - split_x);
                dest[idx] = new_frame[y * SCREEN_WIDTH + src_x];
            } else {
                int src_x = x - split_x;
                dest[idx] = old_frame[y * SCREEN_WIDTH + src_x];
            }
        }
    }
}

// 9. Diagonal wipe: sweep from top-left to bottom-right with soft edge
void transition_diagonal_wipe(pixel_t *dest, const pixel_t *old_frame,
                              const pixel_t *new_frame, float progress) {
    float edge = progress * (SCREEN_WIDTH + SCREEN_HEIGHT - 2);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            float dist = edge - (x + y);
            float alpha = dist / 8.0f + 0.5f;
            if (alpha < 0.0f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;
            dest[idx] = blend_pixels(old_frame[idx], new_frame[idx], alpha);
        }
    }
}

// 10. Zoom: old frame zooms in and fades, new frame fades in underneath
void transition_zoom(pixel_t *dest, const pixel_t *old_frame,
                     const pixel_t *new_frame, float progress) {
    float scale = 1.0f + progress;
    float inv_scale = 1.0f / scale;
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        int sy = cy + (int)((y - cy) * inv_scale);
        if (sy < 0) sy = 0;
        if (sy >= SCREEN_HEIGHT) sy = SCREEN_HEIGHT - 1;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int sx = cx + (int)((x - cx) * inv_scale);
            if (sx < 0) sx = 0;
            if (sx >= SCREEN_WIDTH) sx = SCREEN_WIDTH - 1;
            pixel_t old_sample = old_frame[sy * SCREEN_WIDTH + sx];
            dest[y * SCREEN_WIDTH + x] = blend_pixels(old_sample, new_frame[y * SCREEN_WIDTH + x], progress);
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
        case TRANSITION_FLASH:
            transition_flash(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_CHECKERBOARD:
            transition_checkerboard(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_SCANLINE:
            transition_scanline(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_SLIDE:
            transition_slide(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_DIAGONAL_WIPE:
            transition_diagonal_wipe(dest, old_frame, new_frame, progress);
            break;
        case TRANSITION_ZOOM:
            transition_zoom(dest, old_frame, new_frame, progress);
            break;
        default:
            // Fallback to crossfade
            transition_crossfade(dest, old_frame, new_frame, progress);
            break;
    }
}
