#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "common.h"
#include "plasma.h"
#include "fire.h"
#include "tunnel.h"
#include "starfield.h"
#include "scroller.h"
#include "cube.h"
#include "torus.h"
#include "raster.h"
#include "twister.h"
#include "rotozoom.h"
#include "metaballs.h"
#include "dottunnel.h"
#include "vectorballs.h"
#include "textwriter.h"
#include "synth.h"
#include "sinescroller_large.h"
#include "metaballs3d.h"
#include "ripple.h"
#include "voxel.h"
#include "bumpmap.h"
#include "kaleidoscope.h"
#include "raytracer.h"
#include "sierpinski.h"
#include "particles.h"
#include "tesseract.h"
#include "matrix.h"
#include "matrixcode.h"
#include "lens.h"
#include "voronoi.h"
#include "menu.h"
#include "font.h"
#include "transitions.h"

// Application state
typedef enum {
    STATE_MENU,
    STATE_RUNNING
} AppState;

// Transition state
typedef enum {
    TRANSITION_NONE,
    TRANSITION_IN_PROGRESS
} TransitionState;

// Effect function pointer structure
typedef struct {
    void (*init)(void);
    void (*update)(pixel_t*, uint32_t);
    void (*cleanup)(void);
    const char *name;
} Effect;

// Number of effects
#define NUM_EFFECTS 28

// Effects array
static const Effect effects[NUM_EFFECTS] = {
    {plasma_init, plasma_update, plasma_cleanup, "Plasma"},
    {fire_init, fire_update, fire_cleanup, "Fire"},
    {tunnel_init, tunnel_update, tunnel_cleanup, "Tunnel"},
    {starfield_init, starfield_update, starfield_cleanup, "Starfield"},
    {scroller_init, scroller_update, scroller_cleanup, "Scroller"},
    {cube_init, cube_update, cube_cleanup, "Cube"},
    {torus_init, torus_update, torus_cleanup, "Torus"},
    {raster_init, raster_update, raster_cleanup, "Raster"},
    {twister_init, twister_update, twister_cleanup, "Twister"},
    {rotozoom_init, rotozoom_update, rotozoom_cleanup, "Rotozoom"},
    {metaballs_init, metaballs_update, metaballs_cleanup, "Metaballs"},
    {dottunnel_init, dottunnel_update, dottunnel_cleanup, "Dot Tunnel"},
    {vectorballs_init, vectorballs_update, vectorballs_cleanup, "Vector Balls"},
    {textwriter_init, textwriter_update, textwriter_cleanup, "Text Writer"},
    {sinescroller_large_init, sinescroller_large_update, sinescroller_large_cleanup, "Sine Scroller"},
    {metaballs3d_init, metaballs3d_update, metaballs3d_cleanup, "Metaballs 3D"},
    {ripple_init, ripple_update, ripple_cleanup, "Ripple"},
    {voxel_init, voxel_update, voxel_cleanup, "Voxel"},
    {bumpmap_init, bumpmap_update, bumpmap_cleanup, "Bump Map"},
    {kaleidoscope_init, kaleidoscope_update, kaleidoscope_cleanup, "Kaleidoscope"},
    {raytracer_init, raytracer_update, raytracer_cleanup, "Raytracer"},
    {sierpinski_init, sierpinski_update, sierpinski_cleanup, "Sierpinski"},
    {particles_init, particles_update, particles_cleanup, "Particles"},
    {tesseract_init, tesseract_update, tesseract_cleanup, "Tesseract"},
    {matrix_init, matrix_update, matrix_cleanup, "Matrix"},
    {matrixcode_init, matrixcode_update, matrixcode_cleanup, "Matrix Code"},
    {lens_init, lens_update, lens_cleanup, "Lens"},
    {voronoi_init, voronoi_update, voronoi_cleanup, "Voronoi"}
};

// Lookup tables
int sine_table[256];
int cosine_table[256];

// Initialize common lookup tables
void init_tables(void) {
    for (int i = 0; i < 256; i++) {
        sine_table[i] = (int)(128 + 127 * sin(i * 2 * M_PI / 256));
        cosine_table[i] = (int)(128 + 127 * cos(i * 2 * M_PI / 256));
    }
}

// Draw a line using Bresenham's algorithm
void draw_line(pixel_t *pixels, int x0, int y0, int x1, int y1, pixel_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // Bounds checking
        if (x0 >= 0 && x0 < SCREEN_WIDTH && y0 >= 0 && y0 < SCREEN_HEIGHT) {
            pixels[y0 * SCREEN_WIDTH + x0] = color;
        }

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Save screenshot to file
void save_screenshot(pixel_t *pixels, const char *effect_name) {
    // Create screenshots directory if it doesn't exist
    struct stat st = {0};
    if (stat("screenshots", &st) == -1) {
        mkdir("screenshots", 0755);
    }

    // Generate filename with timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[256];
    snprintf(filename, sizeof(filename),
             "screenshots/%s_%04d%02d%02d_%02d%02d%02d.bmp",
             effect_name,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    // Create SDL surface from pixel buffer
    // Convert to 24-bit format to avoid alpha channel issues in BMP
    SDL_Surface *temp_surface = SDL_CreateRGBSurfaceFrom(
        pixels,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        32,
        SCREEN_WIDTH * sizeof(pixel_t),
        0xFF000000,  // R mask (bits 24-31)
        0x00FF0000,  // G mask (bits 16-23)
        0x0000FF00,  // B mask (bits 8-15)
        0x000000FF   // A mask (bits 0-7)
    );

    if (!temp_surface) {
        fprintf(stderr, "Failed to create surface for screenshot: %s\n", SDL_GetError());
        return;
    }

    // Convert to 24-bit RGB format (no alpha) for BMP compatibility
    SDL_Surface *surface = SDL_ConvertSurfaceFormat(temp_surface, SDL_PIXELFORMAT_RGB24, 0);

    if (!surface) {
        fprintf(stderr, "Failed to convert surface for screenshot: %s\n", SDL_GetError());
        SDL_FreeSurface(temp_surface);
        return;
    }

    // Save as BMP
    if (SDL_SaveBMP(surface, filename) == 0) {
        printf("Screenshot saved: %s\n", filename);
    } else {
        fprintf(stderr, "Failed to save screenshot: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(surface);
    SDL_FreeSurface(temp_surface);
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    int enable_music = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--music") == 0) {
            enable_music = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("DemoFX - Old School Demo Effects\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --music    Enable background music (chip-tune synthesizer)\n");
            printf("  --help     Show this help message\n");
            return 0;
        }
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow(
        "Old School Demo Effects",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * SCALE_FACTOR,
        SCREEN_HEIGHT * SCALE_FACTOR,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create texture for pixel buffer
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    if (!texture) {
        fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Allocate pixel buffer
    pixel_t *pixels = (pixel_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
    if (!pixels) {
        fprintf(stderr, "Memory allocation failed\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Allocate transition buffers
    pixel_t *transition_buffer = (pixel_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
    pixel_t *temp_buffer = (pixel_t *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
    if (!transition_buffer || !temp_buffer) {
        fprintf(stderr, "Transition buffer allocation failed\n");
        free(pixels);
        if (transition_buffer) free(transition_buffer);
        if (temp_buffer) free(temp_buffer);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize lookup tables
    init_tables();

    // Initialize all effects
    for (int i = 0; i < NUM_EFFECTS; i++) {
        effects[i].init();
    }

    // Extract effect names for menu
    static const char* effect_names[NUM_EFFECTS];
    for (int i = 0; i < NUM_EFFECTS; i++) {
        effect_names[i] = effects[i].name;
    }

    // Initialize menu system
    menu_init(effect_names, NUM_EFFECTS);

    // Initialize synthesizer (background music) if enabled
    if (enable_music) {
        synth_init();
        printf("Background music enabled\n");
    }

    // Application state
    AppState app_state = STATE_MENU;
    int selected_effect = 0;  // Currently highlighted effect in menu
    int current_effect = 0;   // Currently running effect
    int is_fullscreen = 0;    // Fullscreen state

    // Transition state
    TransitionState transition_state = TRANSITION_NONE;
    TransitionType current_transition = TRANSITION_CROSSFADE;
    int next_effect = 0;          // Effect to transition to
    Uint32 transition_start_time = 0;
    const Uint32 TRANSITION_DURATION = 500;  // milliseconds
    int show_transition_name = 0;  // Show transition name briefly
    Uint32 transition_name_time = 0;

    // Main loop
    int running = 1;
    SDL_Event event;
    Uint32 frame_time = 0;

    printf("DemoFX - Navigate with UP/DOWN arrows, press ENTER to select\n");
    printf("         Press LEFT/RIGHT arrows to switch effects while running\n");
    printf("         Press T to cycle transition types\n");
    printf("         Press ESC to return to menu or quit\n");
    printf("         Press ALT+ENTER to toggle fullscreen\n");
    printf("         Press F12 to save a screenshot\n");

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                // Check for Alt+Enter to toggle fullscreen
                if (event.key.keysym.sym == SDLK_RETURN &&
                    (event.key.keysym.mod & KMOD_ALT)) {
                    is_fullscreen = !is_fullscreen;
                    if (is_fullscreen) {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        printf("Fullscreen mode enabled\n");
                    } else {
                        SDL_SetWindowFullscreen(window, 0);
                        printf("Windowed mode enabled\n");
                    }
                }
                // Check for F12 to save screenshot
                else if (event.key.keysym.sym == SDLK_F12) {
                    int effect = (app_state == STATE_MENU) ? selected_effect : current_effect;
                    const char *effect_name = menu_get_effect_name(effect);
                    save_screenshot(pixels, effect_name);
                }
                else if (app_state == STATE_MENU) {
                    // Menu navigation
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        case SDLK_UP:
                            if (selected_effect > 0) {
                                selected_effect--;
                            }
                            break;
                        case SDLK_DOWN:
                            if (selected_effect < NUM_EFFECTS - 1) {
                                selected_effect++;
                            }
                            break;
                        case SDLK_RETURN:
                        case SDLK_SPACE:
                            // Launch selected effect
                            current_effect = selected_effect;
                            app_state = STATE_RUNNING;
                            printf("Running: %s\n", menu_get_effect_name(current_effect));
                            break;
                    }
                } else {
                    // Running effect - ESC, LEFT/RIGHT arrows, T key
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            app_state = STATE_MENU;
                            printf("Returned to menu\n");
                            break;
                        case SDLK_LEFT:
                            // Previous effect (with wraparound)
                            if (transition_state == TRANSITION_NONE) {
                                next_effect = (current_effect - 1 + NUM_EFFECTS) % NUM_EFFECTS;
                                memcpy(transition_buffer, pixels, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
                                transition_state = TRANSITION_IN_PROGRESS;
                                transition_start_time = SDL_GetTicks();
                                printf("Transitioning to: %s\n", menu_get_effect_name(next_effect));
                            }
                            break;
                        case SDLK_RIGHT:
                            // Next effect (with wraparound)
                            if (transition_state == TRANSITION_NONE) {
                                next_effect = (current_effect + 1) % NUM_EFFECTS;
                                memcpy(transition_buffer, pixels, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(pixel_t));
                                transition_state = TRANSITION_IN_PROGRESS;
                                transition_start_time = SDL_GetTicks();
                                printf("Transitioning to: %s\n", menu_get_effect_name(next_effect));
                            }
                            break;
                        case SDLK_t:
                            // Cycle transition type
                            current_transition = (TransitionType)((current_transition + 1) % TRANSITION_COUNT);
                            printf("Transition type: %s\n", transition_names[current_transition]);
                            show_transition_name = 1;
                            transition_name_time = SDL_GetTicks();
                            break;
                    }
                }
            }
        }

        // Update and render
        frame_time = SDL_GetTicks();

        // Handle transitions
        if (transition_state == TRANSITION_IN_PROGRESS) {
            Uint32 elapsed = frame_time - transition_start_time;
            float progress = (float)elapsed / TRANSITION_DURATION;

            if (progress >= 1.0f) {
                // Transition complete
                progress = 1.0f;
                transition_state = TRANSITION_NONE;
                current_effect = next_effect;
                printf("Transition complete: %s\n", menu_get_effect_name(current_effect));
            }

            // Render new effect to temp buffer
            effects[next_effect].update(temp_buffer, frame_time);

            // Apply transition blend
            transition_apply(current_transition, pixels, transition_buffer, temp_buffer, progress);
        } else {
            // Normal rendering (no transition)
            // Render the currently selected/running effect as background
            int effect_to_render = (app_state == STATE_MENU) ? selected_effect : current_effect;
            effects[effect_to_render].update(pixels, frame_time);
        }

        // Draw menu overlay if in menu state
        if (app_state == STATE_MENU) {
            menu_draw(pixels, selected_effect);
        }

        // Update texture and render
        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(pixel_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Cap frame rate (~60 FPS)
        SDL_Delay(16);
    }

    // Cleanup all effects
    for (int i = 0; i < NUM_EFFECTS; i++) {
        effects[i].cleanup();
    }
    menu_cleanup();
    synth_cleanup();

    free(pixels);
    free(transition_buffer);
    free(temp_buffer);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
