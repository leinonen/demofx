#include "raytracer.h"
#include <math.h>
#include <float.h>

// Vector math structures
typedef struct {
    float x, y, z;
} vec3_t;

// Ray structure
typedef struct {
    vec3_t origin;
    vec3_t direction;
} ray_t;

// Sphere structure
typedef struct {
    vec3_t center;
    float radius;
    int r, g, b;
    float reflectivity;
} sphere_t;

// Plane structure
typedef struct {
    vec3_t normal;
    float distance;
    int r, g, b;
    int checkerboard;
} plane_t;

// Hit information
typedef struct {
    float t;
    vec3_t point;
    vec3_t normal;
    int r, g, b;
    float reflectivity;
    int hit;
} hit_t;

// Scene objects
static sphere_t spheres[2];
static plane_t floor_plane;
static plane_t ceiling_plane;
static vec3_t light_pos;
static vec3_t camera_pos;

// Fast inverse square root (Quake III cheat!)
static inline float fast_inv_sqrt(float x) {
    float xhalf = 0.5f * x;
    union {
        float f;
        int i;
    } u;
    u.f = x;
    u.i = 0x5f3759df - (u.i >> 1);
    u.f = u.f * (1.5f - xhalf * u.f * u.f);
    return u.f;
}

// Vector operations (inlined for performance)
static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static inline float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3_t vec3_normalize(vec3_t v) {
    float inv_len = fast_inv_sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return vec3_scale(v, inv_len);
}

static inline vec3_t vec3_reflect(vec3_t v, vec3_t n) {
    return vec3_sub(v, vec3_scale(n, 2.0f * vec3_dot(v, n)));
}

// Ray-sphere intersection
static int intersect_sphere(ray_t *ray, sphere_t *sphere, hit_t *hit) {
    vec3_t oc = vec3_sub(ray->origin, sphere->center);
    float a = vec3_dot(ray->direction, ray->direction);
    float b = 2.0f * vec3_dot(oc, ray->direction);
    float c = vec3_dot(oc, oc) - sphere->radius * sphere->radius;
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0) return 0;

    float t = (-b - sqrtf(discriminant)) / (2.0f * a);
    if (t < 0.001f) return 0;  // Too close (avoid self-intersection)

    if (t < hit->t) {
        hit->t = t;
        hit->point = vec3_add(ray->origin, vec3_scale(ray->direction, t));
        hit->normal = vec3_normalize(vec3_sub(hit->point, sphere->center));
        hit->r = sphere->r;
        hit->g = sphere->g;
        hit->b = sphere->b;
        hit->reflectivity = sphere->reflectivity;
        hit->hit = 1;
        return 1;
    }
    return 0;
}

// Ray-plane intersection
static int intersect_plane(ray_t *ray, plane_t *plane, hit_t *hit) {
    float denom = vec3_dot(plane->normal, ray->direction);
    if (fabsf(denom) < 0.0001f) return 0;

    float t = -(vec3_dot(plane->normal, ray->origin) + plane->distance) / denom;
    if (t < 0.001f) return 0;

    if (t < hit->t) {
        hit->t = t;
        hit->point = vec3_add(ray->origin, vec3_scale(ray->direction, t));
        hit->normal = plane->normal;

        // Checkerboard pattern
        if (plane->checkerboard) {
            int ix = (int)(floorf(hit->point.x + 100.0f));
            int iz = (int)(floorf(hit->point.z + 100.0f));
            int checker = (ix + iz) & 1;
            if (checker) {
                hit->r = 200;
                hit->g = 200;
                hit->b = 200;
            } else {
                hit->r = 50;
                hit->g = 50;
                hit->b = 50;
            }
        } else {
            hit->r = plane->r;
            hit->g = plane->g;
            hit->b = plane->b;
        }

        hit->reflectivity = 0.15f;  // Slight reflection on floor/ceiling
        hit->hit = 1;
        return 1;
    }
    return 0;
}

// Trace a ray and return color
static void trace_ray(ray_t *ray, int depth, int *out_r, int *out_g, int *out_b) {
    if (depth > 2) {  // Max 2 bounces (cheat for performance)
        *out_r = 20;
        *out_g = 20;
        *out_b = 30;
        return;
    }

    hit_t hit = {FLT_MAX, {0, 0, 0}, {0, 0, 0}, 0, 0, 0, 0.0f, 0};

    // Test intersection with all objects
    intersect_sphere(ray, &spheres[0], &hit);
    intersect_sphere(ray, &spheres[1], &hit);
    intersect_plane(ray, &floor_plane, &hit);
    intersect_plane(ray, &ceiling_plane, &hit);

    if (!hit.hit) {
        // Sky color gradient
        float t = 0.5f * (ray->direction.y + 1.0f);
        *out_r = (int)(20 + t * 30);
        *out_g = (int)(20 + t * 40);
        *out_b = (int)(30 + t * 50);
        return;
    }

    // Simple Phong lighting
    vec3_t light_dir = vec3_normalize(vec3_sub(light_pos, hit.point));
    float diffuse = fmaxf(vec3_dot(hit.normal, light_dir), 0.0f);

    // Specular highlight
    vec3_t view_dir = vec3_normalize(vec3_sub(camera_pos, hit.point));
    vec3_t reflect_dir = vec3_reflect(vec3_scale(light_dir, -1.0f), hit.normal);
    float spec = powf(fmaxf(vec3_dot(view_dir, reflect_dir), 0.0f), 32.0f);

    // Ambient + diffuse + specular
    float ambient = 0.2f;
    float r = hit.r * (ambient + diffuse * 0.7f) + spec * 255.0f;
    float g = hit.g * (ambient + diffuse * 0.7f) + spec * 255.0f;
    float b = hit.b * (ambient + diffuse * 0.7f) + spec * 255.0f;

    // Reflection (if reflective)
    if (hit.reflectivity > 0.01f && depth < 2) {
        ray_t reflect_ray;
        reflect_ray.origin = hit.point;
        reflect_ray.direction = vec3_reflect(ray->direction, hit.normal);

        int refl_r, refl_g, refl_b;
        trace_ray(&reflect_ray, depth + 1, &refl_r, &refl_g, &refl_b);

        // Blend reflection
        r = r * (1.0f - hit.reflectivity) + refl_r * hit.reflectivity;
        g = g * (1.0f - hit.reflectivity) + refl_g * hit.reflectivity;
        b = b * (1.0f - hit.reflectivity) + refl_b * hit.reflectivity;
    }

    // Clamp colors
    *out_r = (int)fminf(r, 255.0f);
    *out_g = (int)fminf(g, 255.0f);
    *out_b = (int)fminf(b, 255.0f);
}

void raytracer_init(void) {
    // Setup camera
    camera_pos = (vec3_t){0.0f, 0.0f, -5.0f};

    // Setup light
    light_pos = (vec3_t){-3.0f, 4.0f, -2.0f};

    // Setup spheres
    spheres[0] = (sphere_t){
        .center = {-1.2f, 0.0f, 2.0f},
        .radius = 1.0f,
        .r = 220,
        .g = 50,
        .b = 50,
        .reflectivity = 0.6f
    };

    spheres[1] = (sphere_t){
        .center = {1.2f, -0.3f, 1.0f},
        .radius = 0.7f,
        .r = 50,
        .g = 100,
        .b = 220,
        .reflectivity = 0.8f
    };

    // Setup floor
    floor_plane = (plane_t){
        .normal = {0.0f, 1.0f, 0.0f},
        .distance = 1.5f,
        .r = 100,
        .g = 100,
        .b = 100,
        .checkerboard = 1
    };

    // Setup ceiling
    ceiling_plane = (plane_t){
        .normal = {0.0f, -1.0f, 0.0f},
        .distance = 1.5f,
        .r = 100,
        .g = 100,
        .b = 120,
        .checkerboard = 0
    };
}

void raytracer_update(pixel_t *pixels, uint32_t time) {
    float t = time * 0.0005f;

    // Animate spheres (subtle motion)
    spheres[0].center.y = sinf(t) * 0.3f;
    spheres[1].center.x = 1.2f + cosf(t * 0.7f) * 0.3f;

    // Animate light
    light_pos.x = -3.0f + sinf(t * 0.5f) * 2.0f;
    light_pos.z = -2.0f + cosf(t * 0.3f) * 1.5f;

    // Camera parameters
    float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
    float fov = 60.0f * M_PI / 180.0f;
    float half_height = tanf(fov * 0.5f);
    float half_width = aspect * half_height;

    // Render each pixel
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Calculate ray direction
            float u = (2.0f * x / SCREEN_WIDTH - 1.0f) * half_width;
            float v = (1.0f - 2.0f * y / SCREEN_HEIGHT) * half_height;

            ray_t ray;
            ray.origin = camera_pos;
            ray.direction = vec3_normalize((vec3_t){u, v, 1.0f});

            int r, g, b;
            trace_ray(&ray, 0, &r, &g, &b);

            pixels[y * SCREEN_WIDTH + x] = RGB(r, g, b);
        }
    }
}

void raytracer_cleanup(void) {
    // Nothing to cleanup (no dynamic allocation)
}
