#include "torus.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

// 3D point structure
typedef struct {
    float x, y, z;
} Point3D;

// 2D point structure
typedef struct {
    int x, y;
} Point2D;

// Triangle structure
typedef struct {
    int v0, v1, v2;      // Vertex indices
    float avg_z;         // Average Z depth for sorting
    Point3D normal;      // Face normal
    int visible;         // 1 if visible (passes backface culling), 0 otherwise
} Triangle;

// Torus parameters
#define MAJOR_RADIUS 1.5f    // Distance from center to tube center
#define MINOR_RADIUS 0.6f    // Tube radius
#define MAJOR_SEGMENTS 24    // Segments around the major circle
#define MINOR_SEGMENTS 16    // Segments around the tube

#define NUM_VERTICES (MAJOR_SEGMENTS * MINOR_SEGMENTS)
#define NUM_TRIANGLES (MAJOR_SEGMENTS * MINOR_SEGMENTS * 2)  // 2 triangles per quad

// Torus vertices and triangles
static Point3D vertices[NUM_VERTICES];
static Point3D rotated[NUM_VERTICES];
static Point2D projected[NUM_VERTICES];
static Triangle triangles[NUM_TRIANGLES];
static Triangle *sorted_triangles[NUM_TRIANGLES];

// Initialize torus vertices and triangles
void torus_init(void) {
    int idx = 0;

    // Generate vertices
    for (int i = 0; i < MAJOR_SEGMENTS; i++) {
        float u = (float)i / MAJOR_SEGMENTS * 2.0f * M_PI;
        for (int j = 0; j < MINOR_SEGMENTS; j++) {
            float v = (float)j / MINOR_SEGMENTS * 2.0f * M_PI;

            // Parametric equations for a torus
            vertices[idx].x = (MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * cos(u);
            vertices[idx].y = (MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * sin(u);
            vertices[idx].z = MINOR_RADIUS * sin(v);

            idx++;
        }
    }

    // Generate triangles (2 per quad)
    int tri_idx = 0;
    for (int i = 0; i < MAJOR_SEGMENTS; i++) {
        for (int j = 0; j < MINOR_SEGMENTS; j++) {
            int v0 = i * MINOR_SEGMENTS + j;
            int v1 = i * MINOR_SEGMENTS + ((j + 1) % MINOR_SEGMENTS);
            int v2 = ((i + 1) % MAJOR_SEGMENTS) * MINOR_SEGMENTS + j;
            int v3 = ((i + 1) % MAJOR_SEGMENTS) * MINOR_SEGMENTS + ((j + 1) % MINOR_SEGMENTS);

            // First triangle (v0, v1, v2)
            triangles[tri_idx].v0 = v0;
            triangles[tri_idx].v1 = v1;
            triangles[tri_idx].v2 = v2;
            tri_idx++;

            // Second triangle (v1, v3, v2)
            triangles[tri_idx].v0 = v1;
            triangles[tri_idx].v1 = v3;
            triangles[tri_idx].v2 = v2;
            tri_idx++;
        }
    }

    // Initialize sorted triangle pointers
    for (int i = 0; i < NUM_TRIANGLES; i++) {
        sorted_triangles[i] = &triangles[i];
    }
}

void torus_cleanup(void) {
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
    float scale = 40.0f;
    float distance = 5.0f;

    float factor = distance / (distance + p3d->z);

    p2d->x = (int)(SCREEN_WIDTH / 2 + p3d->x * scale * factor);
    p2d->y = (int)(SCREEN_HEIGHT / 2 - p3d->y * scale * factor);
}

// Vector cross product
static Point3D cross_product(Point3D *a, Point3D *b) {
    Point3D result;
    result.x = a->y * b->z - a->z * b->y;
    result.y = a->z * b->x - a->x * b->z;
    result.z = a->x * b->y - a->y * b->x;
    return result;
}

// Vector dot product
static float dot_product(Point3D *a, Point3D *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

// Calculate face normal (not normalized for speed)
static void calculate_normal(Triangle *tri) {
    Point3D *p0 = &rotated[tri->v0];
    Point3D *p1 = &rotated[tri->v1];
    Point3D *p2 = &rotated[tri->v2];

    // Edge vectors
    Point3D edge1 = {p1->x - p0->x, p1->y - p0->y, p1->z - p0->z};
    Point3D edge2 = {p2->x - p0->x, p2->y - p0->y, p2->z - p0->z};

    // Normal = edge1 × edge2
    tri->normal = cross_product(&edge1, &edge2);
}

// Check if triangle is visible (backface culling)
static int is_visible(Triangle *tri) {
    // View vector from triangle center to camera (camera at z = -distance)
    Point3D *p0 = &rotated[tri->v0];
    Point3D view = {-p0->x, -p0->y, -5.0f - p0->z};  // Camera at z = -5

    // Backface culling: dot product < 0 means facing camera
    return dot_product(&tri->normal, &view) < 0;
}

// Comparison function for depth sorting (painter's algorithm)
static int compare_triangles(const void *a, const void *b) {
    Triangle *t1 = *(Triangle **)a;
    Triangle *t2 = *(Triangle **)b;

    // Sort by average Z (far to near, so far triangles drawn first)
    if (t1->avg_z > t2->avg_z) return -1;
    if (t1->avg_z < t2->avg_z) return 1;
    return 0;
}

// Draw a filled triangle using scanline rasterization
static void draw_filled_triangle(pixel_t *pixels, Triangle *tri, pixel_t color) {
    Point2D *p0 = &projected[tri->v0];
    Point2D *p1 = &projected[tri->v1];
    Point2D *p2 = &projected[tri->v2];

    // Sort vertices by Y coordinate (p0.y <= p1.y <= p2.y)
    Point2D *temp;
    if (p0->y > p1->y) { temp = p0; p0 = p1; p1 = temp; }
    if (p0->y > p2->y) { temp = p0; p0 = p2; p2 = temp; }
    if (p1->y > p2->y) { temp = p1; p1 = p2; p2 = temp; }

    // Degenerate triangle (all points on same line)
    if (p0->y == p2->y) return;

    // Helper function to fill scanline
    auto void fill_scanline(int y, int x1, int x2) {
        if (y < 0 || y >= SCREEN_HEIGHT) return;
        if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
        if (x1 < 0) x1 = 0;
        if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;

        for (int x = x1; x <= x2; x++) {
            pixels[y * SCREEN_WIDTH + x] = color;
        }
    }

    // Calculate slopes
    float invslope1 = 0, invslope2 = 0;
    int x1, x2;

    // Draw upper half (from p0 to p1)
    if (p1->y > p0->y) {
        invslope1 = (float)(p1->x - p0->x) / (p1->y - p0->y);
        invslope2 = (float)(p2->x - p0->x) / (p2->y - p0->y);

        float curx1 = p0->x;
        float curx2 = p0->x;

        for (int y = p0->y; y < p1->y; y++) {
            x1 = (int)(curx1 + 0.5f);
            x2 = (int)(curx2 + 0.5f);
            fill_scanline(y, x1, x2);
            curx1 += invslope1;
            curx2 += invslope2;
        }
    }

    // Draw lower half (from p1 to p2)
    if (p2->y > p1->y) {
        invslope1 = (float)(p2->x - p1->x) / (p2->y - p1->y);
        invslope2 = (float)(p2->x - p0->x) / (p2->y - p0->y);

        float curx1 = p1->x;
        float curx2 = p0->x + invslope2 * (p1->y - p0->y);

        for (int y = p1->y; y <= p2->y; y++) {
            x1 = (int)(curx1 + 0.5f);
            x2 = (int)(curx2 + 0.5f);
            fill_scanline(y, x1, x2);
            curx1 += invslope1;
            curx2 += invslope2;
        }
    }
}

void torus_update(pixel_t *pixels, uint32_t time) {
    // Clear screen
    memset(pixels, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));

    // Calculate rotation angles
    float angle_x = time * 0.0004f;
    float angle_y = time * 0.0006f;
    float angle_z = time * 0.0002f;

    // Transform all vertices
    for (int i = 0; i < NUM_VERTICES; i++) {
        rotated[i] = vertices[i];

        rotate_x(&rotated[i], angle_x);
        rotate_y(&rotated[i], angle_y);
        rotate_z(&rotated[i], angle_z);

        project(&rotated[i], &projected[i]);
    }

    // Light direction (normalized, pointing from upper-right-front)
    Point3D light = {0.5f, -0.7f, -0.5f};

    // Process all triangles
    int visible_count = 0;
    for (int i = 0; i < NUM_TRIANGLES; i++) {
        Triangle *tri = &triangles[i];

        // Calculate normal
        calculate_normal(tri);

        // Backface culling
        if (is_visible(tri)) {
            tri->visible = 1;

            // Calculate average Z depth for sorting
            tri->avg_z = (rotated[tri->v0].z + rotated[tri->v1].z + rotated[tri->v2].z) / 3.0f;

            // Add to sorted list
            sorted_triangles[visible_count++] = tri;
        } else {
            tri->visible = 0;
        }
    }

    // Sort visible triangles by depth (painter's algorithm)
    qsort(sorted_triangles, visible_count, sizeof(Triangle*), compare_triangles);

    // Draw all visible triangles back to front
    for (int i = 0; i < visible_count; i++) {
        Triangle *tri = sorted_triangles[i];

        // Calculate lighting (flat shading)
        // Normalize the normal vector
        float normal_len = sqrt(tri->normal.x * tri->normal.x +
                               tri->normal.y * tri->normal.y +
                               tri->normal.z * tri->normal.z);

        if (normal_len > 0.001f) {
            Point3D normalized_normal = {
                tri->normal.x / normal_len,
                tri->normal.y / normal_len,
                tri->normal.z / normal_len
            };

            // Diffuse lighting: dot product with light direction
            float diffuse = dot_product(&normalized_normal, &light);
            diffuse = diffuse < 0 ? 0 : diffuse;  // Clamp to positive

            // Ambient + diffuse lighting
            float ambient = 0.3f;
            float intensity = ambient + (1.0f - ambient) * diffuse;

            // Color based on intensity (purple/magenta torus)
            int r = (int)(intensity * 200);
            int g = (int)(intensity * 50);
            int b = (int)(intensity * 180);

            // Clamp colors
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;

            pixel_t color = RGB(r, g, b);

            // Draw filled triangle
            draw_filled_triangle(pixels, tri, color);
        }
    }
}
