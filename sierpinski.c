#include "sierpinski.h"
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

// Edge structure for rendering
typedef struct {
    Point3D v0, v1;  // Two vertices of the edge
    int level;       // Recursion level (for coloring)
} Edge;

#define MAX_EDGES 512
static Edge edges[MAX_EDGES];
static int edge_count = 0;

void sierpinski_init(void) {
    edge_count = 0;
}

void sierpinski_cleanup(void) {
    // Nothing to clean up
}

// Add an edge to the edge list
static void add_edge(Point3D v0, Point3D v1, int level) {
    if (edge_count < MAX_EDGES) {
        edges[edge_count].v0 = v0;
        edges[edge_count].v1 = v1;
        edges[edge_count].level = level;
        edge_count++;
    }
}

// Calculate midpoint between two points
static Point3D midpoint(Point3D a, Point3D b) {
    Point3D mid;
    mid.x = (a.x + b.x) / 2.0f;
    mid.y = (a.y + b.y) / 2.0f;
    mid.z = (a.z + b.z) / 2.0f;
    return mid;
}

// Recursive function to generate Sierpinski tetrahedron
static void generate_sierpinski(Point3D v0, Point3D v1, Point3D v2, Point3D v3, int level, int max_level) {
    if (level == 0) {
        // Base case: draw the tetrahedron edges
        add_edge(v0, v1, max_level);
        add_edge(v0, v2, max_level);
        add_edge(v0, v3, max_level);
        add_edge(v1, v2, max_level);
        add_edge(v1, v3, max_level);
        add_edge(v2, v3, max_level);
    } else {
        // Recursive case: subdivide into 4 smaller tetrahedra
        // Calculate midpoints of all edges
        Point3D m01 = midpoint(v0, v1);
        Point3D m02 = midpoint(v0, v2);
        Point3D m03 = midpoint(v0, v3);
        Point3D m12 = midpoint(v1, v2);
        Point3D m13 = midpoint(v1, v3);
        Point3D m23 = midpoint(v2, v3);

        // Create 4 smaller tetrahedra (corners of original)
        generate_sierpinski(v0, m01, m02, m03, level - 1, max_level);
        generate_sierpinski(m01, v1, m12, m13, level - 1, max_level);
        generate_sierpinski(m02, m12, v2, m23, level - 1, max_level);
        generate_sierpinski(m03, m13, m23, v3, level - 1, max_level);
    }
}

// Rotate a point around X axis
static void rotate_x(Point3D *p, float angle) {
    float y = p->y;
    float z = p->z;
    p->y = y * cosf(angle) - z * sinf(angle);
    p->z = y * sinf(angle) + z * cosf(angle);
}

// Rotate a point around Y axis
static void rotate_y(Point3D *p, float angle) {
    float x = p->x;
    float z = p->z;
    p->x = x * cosf(angle) + z * sinf(angle);
    p->z = -x * sinf(angle) + z * cosf(angle);
}

// Rotate a point around Z axis
static void rotate_z(Point3D *p, float angle) {
    float x = p->x;
    float y = p->y;
    p->x = x * cosf(angle) - y * sinf(angle);
    p->y = x * sinf(angle) + y * cosf(angle);
}

// Project 3D point to 2D screen coordinates
static void project(Point3D *p3d, Point2D *p2d) {
    // Perspective projection
    float scale = 60.0f;    // Size on screen
    float distance = 5.0f;  // Distance from camera

    // Perspective projection factor
    float factor = distance / (distance + p3d->z);

    p2d->x = (int)(SCREEN_WIDTH / 2 + p3d->x * scale * factor);
    p2d->y = (int)(SCREEN_HEIGHT / 2 - p3d->y * scale * factor);
}

// Get warm fire glow color based on depth and level
static pixel_t get_glow_color(float z, int level) {
    // Normalize depth to 0-1 range (closer = higher value)
    float normalized_z = (z + 3.0f) / 6.0f;
    normalized_z = normalized_z < 0.0f ? 0.0f : (normalized_z > 1.0f ? 1.0f : normalized_z);
    normalized_z = 1.0f - normalized_z;  // Invert so closer is brighter

    // Base intensity from depth
    float intensity = 0.5f + 0.5f * normalized_z;

    // Level-based color variation (deeper recursion = more intense)
    float level_factor = (level + 1) / 4.0f;
    intensity *= (0.7f + 0.3f * level_factor);

    // Warm fire glow: yellow -> orange -> red
    // Higher intensity = yellow, lower = deep red
    int r, g, b;

    if (intensity > 0.66f) {
        // Yellow to orange
        float t = (intensity - 0.66f) / 0.34f;
        r = 255;
        g = (int)(150 + 105 * t);  // 150-255
        b = (int)(30 * (1.0f - t)); // 30-0
    } else if (intensity > 0.33f) {
        // Orange to red-orange
        float t = (intensity - 0.33f) / 0.33f;
        r = 255;
        g = (int)(80 + 70 * t);    // 80-150
        b = 0;
    } else {
        // Dark red to red-orange
        float t = intensity / 0.33f;
        r = (int)(120 + 135 * t);  // 120-255
        g = (int)(80 * t);         // 0-80
        b = 0;
    }

    return RGB(r, g, b);
}

void sierpinski_update(pixel_t *pixels, uint32_t time) {
    // Clear screen
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Reset edge count
    edge_count = 0;

    // Define base tetrahedron vertices (regular tetrahedron centered at origin)
    // Using coordinates that form a regular tetrahedron
    float h = 1.0f / sqrtf(2.0f / 3.0f);  // Height adjustment for regular tetrahedron
    Point3D v0 = { 0.0f,  h * 0.7f, 0.0f};                  // Top vertex
    Point3D v1 = { 1.0f, -h * 0.3f,  1.0f};                 // Bottom-front-right
    Point3D v2 = {-1.0f, -h * 0.3f,  1.0f};                 // Bottom-front-left
    Point3D v3 = { 0.0f, -h * 0.3f, -1.4f};                 // Bottom-back

    // Generate Sierpinski structure (level 3 = 64 pyramids)
    generate_sierpinski(v0, v1, v2, v3, 3, 3);

    // Calculate rotation angles based on time (smooth rotation)
    float angle_x = time * 0.0004f;
    float angle_y = time * 0.0006f;
    float angle_z = time * 0.0005f;

    // Transform and draw all edges
    for (int i = 0; i < edge_count; i++) {
        // Copy and rotate both vertices of the edge
        Point3D p0 = edges[i].v0;
        Point3D p1 = edges[i].v1;

        // Apply rotations to both vertices
        rotate_x(&p0, angle_x);
        rotate_y(&p0, angle_y);
        rotate_z(&p0, angle_z);

        rotate_x(&p1, angle_x);
        rotate_y(&p1, angle_y);
        rotate_z(&p1, angle_z);

        // Project to 2D
        Point2D proj0, proj1;
        project(&p0, &proj0);
        project(&p1, &proj1);

        // Calculate average depth for color
        float avg_z = (p0.z + p1.z) / 2.0f;

        // Get warm glow color
        pixel_t color = get_glow_color(avg_z, edges[i].level);

        // Draw the edge
        draw_line(pixels, proj0.x, proj0.y, proj1.x, proj1.y, color);
    }
}
