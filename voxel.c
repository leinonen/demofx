#include "voxel.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MAP_SIZE 1024
#define MAP_MASK (MAP_SIZE - 1)

static unsigned char *height_map;
static pixel_t *color_map;

/* Generate procedural heightmap using multiple sine waves */
static void generate_heightmap(void) {
    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            /* Combine multiple sine waves at different frequencies */
            float height = 0;

            /* Large rolling hills */
            height += 30.0f * sin(x * 0.01f) * cos(y * 0.01f);

            /* Medium hills */
            height += 20.0f * sin(x * 0.03f + y * 0.03f);

            /* Small details */
            height += 10.0f * sin(x * 0.1f) * sin(y * 0.1f);

            /* Add some interference patterns */
            height += 15.0f * sin((x + y) * 0.02f) * cos((x - y) * 0.02f);

            /* Normalize to 0-255 range */
            int h = (int)(height + 128.0f);
            if (h < 0) h = 0;
            if (h > 255) h = 255;

            height_map[y * MAP_SIZE + x] = (unsigned char)h;
        }
    }
}

/* Generate colormap based on height (water, sand, grass, rock, snow) */
static void generate_colormap(void) {
    for (int y = 0; y < MAP_SIZE; y++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            int idx = y * MAP_SIZE + x;
            int height = height_map[idx];

            int r, g, b;

            if (height < 80) {
                /* Water - blue */
                r = 20;
                g = 50 + height / 2;
                b = 120 + height / 2;
            } else if (height < 95) {
                /* Sand - yellow/tan */
                r = 194;
                g = 178;
                b = 128;
            } else if (height < 140) {
                /* Grass - green with variation */
                r = 34 + (height - 95) / 2;
                g = 139 - (height - 95) / 3;
                b = 34;
            } else if (height < 180) {
                /* Rock - gray/brown */
                r = 100 + (height - 140);
                g = 80 + (height - 140);
                b = 60 + (height - 140) / 2;
            } else {
                /* Snow - white */
                r = 220 + (height - 180) / 2;
                g = 220 + (height - 180) / 2;
                b = 240;
            }

            /* Add some texture variation based on position */
            r += ((x * 7 + y * 13) % 20) - 10;
            g += ((x * 11 + y * 5) % 20) - 10;
            b += ((x * 13 + y * 7) % 20) - 10;

            /* Clamp */
            if (r < 0) r = 0;
            if (r > 255) r = 255;
            if (g < 0) g = 0;
            if (g > 255) g = 255;
            if (b < 0) b = 0;
            if (b > 255) b = 255;

            color_map[idx] = RGB(r, g, b);
        }
    }
}

void voxel_init(void) {
    height_map = (unsigned char *)malloc(MAP_SIZE * MAP_SIZE);
    if (!height_map) {
        fprintf(stderr, "Failed to allocate voxel height map (%d bytes)\n", MAP_SIZE * MAP_SIZE);
        return;
    }

    color_map = (pixel_t *)malloc(MAP_SIZE * MAP_SIZE * sizeof(pixel_t));
    if (!color_map) {
        fprintf(stderr, "Failed to allocate voxel color map (%zu bytes)\n",
                MAP_SIZE * MAP_SIZE * sizeof(pixel_t));
        free(height_map);
        height_map = NULL;
        return;
    }

    generate_heightmap();
    generate_colormap();
}

/* Voxel space raycasting renderer */
void voxel_update(pixel_t *pixels, uint32_t time) {
    /* Check if initialization succeeded */
    if (!height_map || !color_map) {
        /* Clear screen to black on error */
        memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
        return;
    }

    /* Camera parameters */
    float camera_x = 512.0f + 400.0f * sin(time * 0.0003f);
    float camera_y = 512.0f + 400.0f * cos(time * 0.0003f);
    float camera_height = 220.0f + 30.0f * sin(time * 0.001f);
    float camera_angle = time * 0.0005f;
    float camera_horizon = 100.0f;
    float scale_height = 120.0f;

    /* Sky gradient */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        int sky_brightness = 100 + (SCREEN_HEIGHT - y) * 155 / SCREEN_HEIGHT;
        pixel_t sky_color = RGB(
            sky_brightness / 2,
            sky_brightness / 2 + 20,
            sky_brightness
        );
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            pixels[y * SCREEN_WIDTH + x] = sky_color;
        }
    }

    /* Precompute sin/cos for camera rotation */
    float sin_angle = sinf(camera_angle);
    float cos_angle = cosf(camera_angle);

    /* Cast rays for each screen column */
    for (int screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
        /* Ray direction */
        float ray_dir_x = (screen_x - SCREEN_WIDTH / 2.0f) / SCREEN_WIDTH;

        /* Rotate ray direction */
        float dir_x = cos_angle - ray_dir_x * sin_angle;
        float dir_y = sin_angle + ray_dir_x * cos_angle;

        /* Maximum y coordinate drawn so far (horizon culling) */
        float max_y = SCREEN_HEIGHT;

        /* Ray marching parameters */
        float depth = 1.0f;
        float depth_step = 1.0f;
        float max_depth = 800.0f;

        /* March along the ray */
        while (depth < max_depth) {
            /* Sample position in map */
            float map_x = camera_x + dir_x * depth;
            float map_y = camera_y + dir_y * depth;

            /* Wrap around map */
            int sample_x = ((int)map_x) & MAP_MASK;
            int sample_y = ((int)map_y) & MAP_MASK;

            /* Get height from heightmap */
            int map_height = height_map[sample_y * MAP_SIZE + sample_x];

            /* Project height to screen space */
            float height_on_screen = (camera_height - map_height) / depth * scale_height + camera_horizon;

            /* Draw vertical line if visible */
            if (height_on_screen < max_y) {
                /* Get color from colormap */
                pixel_t color = color_map[sample_y * MAP_SIZE + sample_x];

                /* Apply distance fog */
                float fog = depth / max_depth;
                if (fog > 1.0f) fog = 1.0f;

                int r = ((color >> 16) & 0xFF);
                int g = ((color >> 8) & 0xFF);
                int b = (color & 0xFF);

                /* Mix with sky color based on distance */
                int sky_brightness = 180;
                r = (int)(r * (1.0f - fog) + sky_brightness / 2 * fog);
                g = (int)(g * (1.0f - fog) + (sky_brightness / 2 + 20) * fog);
                b = (int)(b * (1.0f - fog) + sky_brightness * fog);

                pixel_t final_color = RGB(r, g, b);

                /* Draw vertical line from height_on_screen to max_y */
                int start_y = (int)height_on_screen;
                int end_y = (int)max_y;

                if (start_y < 0) start_y = 0;
                if (end_y > SCREEN_HEIGHT) end_y = SCREEN_HEIGHT;

                for (int y = start_y; y < end_y; y++) {
                    pixels[y * SCREEN_WIDTH + screen_x] = final_color;
                }

                /* Update maximum y */
                max_y = height_on_screen;

                /* Stop if we've reached the top of the screen */
                if (max_y < 0) break;
            }

            /* Step forward along ray */
            depth += depth_step;

            /* Increase step size with distance for performance */
            depth_step += 0.05f;
        }
    }
}

void voxel_cleanup(void) {
    free(height_map);
    free(color_map);
}
