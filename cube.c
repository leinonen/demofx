#include "cube.h"
#include <string.h>
#include <math.h>

// 3D point structure
typedef struct {
    float x, y, z;
} Point3D;

// 2D point structure
typedef struct {
    int x, y;
} Point2D;

// Cube vertices (unit cube centered at origin)
static Point3D vertices[8] = {
    {-1, -1, -1},  // 0
    { 1, -1, -1},  // 1
    { 1,  1, -1},  // 2
    {-1,  1, -1},  // 3
    {-1, -1,  1},  // 4
    { 1, -1,  1},  // 5
    { 1,  1,  1},  // 6
    {-1,  1,  1}   // 7
};

// Cube edges (pairs of vertex indices)
static int edges[12][2] = {
    // Back face
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    // Front face
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    // Connecting edges
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

// Rotated vertices (transformed each frame)
static Point3D rotated[8];
static Point2D projected[8];

void cube_init(void) {
    // Nothing to initialize
}

void cube_cleanup(void) {
    // Nothing to clean up
}

// Rotate a point around X axis
static void rotate_x(Point3D *p, float angle) {
    float y = p->y;
    float z = p->z;
    p->y = y * cos(angle) - z * sin(angle);
    p->z = y * sin(angle) + z * cos(angle);
}

// Rotate a point around Y axis
static void rotate_y(Point3D *p, float angle) {
    float x = p->x;
    float z = p->z;
    p->x = x * cos(angle) + z * sin(angle);
    p->z = -x * sin(angle) + z * cos(angle);
}

// Rotate a point around Z axis
static void rotate_z(Point3D *p, float angle) {
    float x = p->x;
    float y = p->y;
    p->x = x * cos(angle) - y * sin(angle);
    p->y = x * sin(angle) + y * cos(angle);
}

// Project 3D point to 2D screen coordinates
static void project(Point3D *p3d, Point2D *p2d) {
    // Simple perspective projection
    float scale = 50.0f;  // Size of cube on screen
    float distance = 5.0f; // Distance from camera

    // Perspective projection factor
    float factor = distance / (distance + p3d->z);

    p2d->x = (int)(SCREEN_WIDTH / 2 + p3d->x * scale * factor);
    p2d->y = (int)(SCREEN_HEIGHT / 2 - p3d->y * scale * factor);
}

void cube_update(pixel_t *pixels, uint32_t time) {
    // Clear screen
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Calculate rotation angles based on time (slow rotation)
    float angle_x = time * 0.0005f;
    float angle_y = time * 0.0007f;
    float angle_z = time * 0.0003f;

    // Transform all vertices
    for (int i = 0; i < 8; i++) {
        // Copy original vertex
        rotated[i] = vertices[i];

        // Apply rotations
        rotate_x(&rotated[i], angle_x);
        rotate_y(&rotated[i], angle_y);
        rotate_z(&rotated[i], angle_z);

        // Project to 2D
        project(&rotated[i], &projected[i]);
    }

    // Draw all edges with depth-based coloring
    for (int i = 0; i < 12; i++) {
        int v0 = edges[i][0];
        int v1 = edges[i][1];

        // Calculate average depth (Z coordinate) of the edge
        float avg_z = (rotated[v0].z + rotated[v1].z) / 2.0f;

        // Map depth to brightness (closer = brighter)
        // Z ranges roughly from -1.7 to +1.7 after rotation
        // Negative Z = closer, Positive Z = farther
        float normalized_z = (avg_z + 2.0f) / 4.0f;  // Normalize to 0-1 range
        normalized_z = normalized_z < 0.0f ? 0.0f : (normalized_z > 1.0f ? 1.0f : normalized_z);
        normalized_z = 1.0f - normalized_z;  // Invert so closer is brighter

        // Create color that fades from dark cyan (far) to bright cyan (near)
        int brightness = (int)(100 + 155 * normalized_z);  // Range: 100-255
        pixel_t color = RGB(0, brightness, brightness);

        draw_line(pixels,
                  projected[v0].x, projected[v0].y,
                  projected[v1].x, projected[v1].y,
                  color);
    }
}
