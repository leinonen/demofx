#include "tesseract.h"
#include <math.h>
#include <string.h>

typedef struct {
    float x, y, z, w;
} Point4D;

typedef struct {
    float x, y, z;
} Point3D;

typedef struct {
    int x, y;
} Point2D;

// Tesseract has 16 vertices (all combinations of ±1 in 4D)
static Point4D vertices[16];
static Point4D rotated[16];
static Point3D projected3d[16];
static Point2D projected2d[16];

// 32 edges connecting vertices (each vertex connects to 4 neighbors)
static int edges[32][2];

// Rotate in XY plane
static void rotate_xy(Point4D *p, float angle) {
    float x = p->x;
    float y = p->y;
    p->x = x * cosf(angle) - y * sinf(angle);
    p->y = x * sinf(angle) + y * cosf(angle);
}

// Rotate in XZ plane
static void rotate_xz(Point4D *p, float angle) {
    float x = p->x;
    float z = p->z;
    p->x = x * cosf(angle) - z * sinf(angle);
    p->z = x * sinf(angle) + z * cosf(angle);
}

// Rotate in XW plane (4D rotation!)
static void rotate_xw(Point4D *p, float angle) {
    float x = p->x;
    float w = p->w;
    p->x = x * cosf(angle) - w * sinf(angle);
    p->w = x * sinf(angle) + w * cosf(angle);
}

// Rotate in YZ plane
static void rotate_yz(Point4D *p, float angle) {
    float y = p->y;
    float z = p->z;
    p->y = y * cosf(angle) - z * sinf(angle);
    p->z = y * sinf(angle) + z * cosf(angle);
}

// Rotate in YW plane (4D rotation!)
static void rotate_yw(Point4D *p, float angle) {
    float y = p->y;
    float w = p->w;
    p->y = y * cosf(angle) - w * sinf(angle);
    p->w = y * sinf(angle) + w * cosf(angle);
}

// Rotate in ZW plane (4D rotation!)
static void rotate_zw(Point4D *p, float angle) {
    float z = p->z;
    float w = p->w;
    p->z = z * cosf(angle) - w * sinf(angle);
    p->w = z * sinf(angle) + w * cosf(angle);
}

// Project 4D point to 3D
static void project_4d_to_3d(Point4D *p4d, Point3D *p3d) {
    float w_distance = 3.0f;
    float w_factor = w_distance / (w_distance + p4d->w);

    p3d->x = p4d->x * w_factor;
    p3d->y = p4d->y * w_factor;
    p3d->z = p4d->z * w_factor;
}

// Project 3D point to 2D (perspective projection)
static void project_3d_to_2d(Point3D *p3d, Point2D *p2d) {
    float scale = 48.0f;
    float distance = 4.0f;

    float factor = distance / (distance + p3d->z);

    p2d->x = (int)(SCREEN_WIDTH / 2 + p3d->x * scale * factor);
    p2d->y = (int)(SCREEN_HEIGHT / 2 - p3d->y * scale * factor);
}

void tesseract_init(void) {
    // Initialize 16 vertices of tesseract (all combinations of ±1 in 4D)
    int idx = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                for (int l = 0; l < 2; l++) {
                    vertices[idx].x = (i == 0) ? -1.0f : 1.0f;
                    vertices[idx].y = (j == 0) ? -1.0f : 1.0f;
                    vertices[idx].z = (k == 0) ? -1.0f : 1.0f;
                    vertices[idx].w = (l == 0) ? -1.0f : 1.0f;
                    idx++;
                }
            }
        }
    }

    // Define 32 edges (each vertex connects to 4 neighbors that differ in one coordinate)
    int edge_idx = 0;
    for (int i = 0; i < 16; i++) {
        for (int j = i + 1; j < 16; j++) {
            // Count how many coordinates differ
            int diff_count = 0;
            if (vertices[i].x != vertices[j].x) diff_count++;
            if (vertices[i].y != vertices[j].y) diff_count++;
            if (vertices[i].z != vertices[j].z) diff_count++;
            if (vertices[i].w != vertices[j].w) diff_count++;

            // Connect vertices that differ in exactly one coordinate
            if (diff_count == 1) {
                edges[edge_idx][0] = i;
                edges[edge_idx][1] = j;
                edge_idx++;
            }
        }
    }
}

void tesseract_update(pixel_t *pixels, uint32_t time) {
    // Clear screen
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Calculate rotation angles from time
    float t = time / 1000.0f;
    float angle_xy = t * 0.3f;
    float angle_xz = t * 0.2f;
    float angle_xw = t * 0.15f;
    float angle_yz = t * 0.25f;
    float angle_yw = t * 0.18f;
    float angle_zw = t * 0.22f;

    // Rotate and project all vertices
    for (int i = 0; i < 16; i++) {
        // Copy original vertex
        rotated[i] = vertices[i];

        // Apply 4D rotations (6 possible rotation planes in 4D)
        rotate_xy(&rotated[i], angle_xy);
        rotate_xz(&rotated[i], angle_xz);
        rotate_xw(&rotated[i], angle_xw);
        rotate_yz(&rotated[i], angle_yz);
        rotate_yw(&rotated[i], angle_yw);
        rotate_zw(&rotated[i], angle_zw);

        // Project 4D -> 3D -> 2D
        project_4d_to_3d(&rotated[i], &projected3d[i]);
        project_3d_to_2d(&projected3d[i], &projected2d[i]);
    }

    // Draw all 32 edges
    for (int i = 0; i < 32; i++) {
        int v0 = edges[i][0];
        int v1 = edges[i][1];

        // Calculate average depth (in 3D space after 4D projection)
        float avg_z = (projected3d[v0].z + projected3d[v1].z) / 2.0f;

        // Also consider W coordinate for extra depth cue
        float avg_w = (rotated[v0].w + rotated[v1].w) / 2.0f;

        // Combine Z and W for depth-based coloring
        float depth = (avg_z + avg_w) / 2.0f;
        float normalized_depth = (depth + 2.0f) / 4.0f;
        normalized_depth = 1.0f - normalized_depth;
        if (normalized_depth < 0.0f) normalized_depth = 0.0f;
        if (normalized_depth > 1.0f) normalized_depth = 1.0f;

        // Color based on depth (cyan to bright cyan)
        int brightness = (int)(80 + 175 * normalized_depth);
        int red = (int)(brightness * 0.3f);
        int green = (int)(brightness * 0.9f);
        int blue = brightness;

        pixel_t color = RGB(red, green, blue);

        draw_line(pixels,
                 projected2d[v0].x, projected2d[v0].y,
                 projected2d[v1].x, projected2d[v1].y,
                 color);
    }
}

void tesseract_cleanup(void) {
    // Nothing to cleanup
}
