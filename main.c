#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(int argc, char *argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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

    // Initialize lookup tables
    init_tables();

    // Initialize effects
    plasma_init();
    fire_init();
    tunnel_init();
    starfield_init();
    scroller_init();
    cube_init();
    torus_init();
    raster_init();
    twister_init();
    rotozoom_init();
    metaballs_init();
    dottunnel_init();
    vectorballs_init();
    textwriter_init();

    // Current effect (0 = plasma, 1 = fire, 2 = tunnel, 3 = starfield, 4 = scroller, 5 = cube, 6 = torus, 7 = raster)
    int current_effect = 0;

    // Main loop
    int running = 1;
    SDL_Event event;
    Uint32 frame_time = 0;

    printf("Demo Effects - Controls:\n");
    printf("  0 = Rotozoom\n");
    printf("  1 = Plasma\n");
    printf("  2 = Fire\n");
    printf("  3 = Tunnel\n");
    printf("  4 = Starfield\n");
    printf("  5 = Sine Scroller\n");
    printf("  6 = Rotating Cube\n");
    printf("  7 = Rotating Torus\n");
    printf("  8 = Raster Bars\n");
    printf("  9 = Twister\n");
    printf("  M = Metaballs\n");
    printf("  D = Dot Tunnel\n");
    printf("  V = Vector Balls\n");
    printf("  T = Text Writer\n");
    printf("  ESC = Quit\n");

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_0:
                        current_effect = 9;
                        printf("Switched to Rotozoom effect\n");
                        break;
                    case SDLK_1:
                        current_effect = 0;
                        printf("Switched to Plasma effect\n");
                        break;
                    case SDLK_2:
                        current_effect = 1;
                        printf("Switched to Fire effect\n");
                        break;
                    case SDLK_3:
                        current_effect = 2;
                        printf("Switched to Tunnel effect\n");
                        break;
                    case SDLK_4:
                        current_effect = 3;
                        printf("Switched to Starfield effect\n");
                        break;
                    case SDLK_5:
                        current_effect = 4;
                        printf("Switched to Sine Scroller effect\n");
                        break;
                    case SDLK_6:
                        current_effect = 5;
                        printf("Switched to Rotating Cube effect\n");
                        break;
                    case SDLK_7:
                        current_effect = 6;
                        printf("Switched to Rotating Torus effect\n");
                        break;
                    case SDLK_8:
                        current_effect = 7;
                        printf("Switched to Raster Bars effect\n");
                        break;
                    case SDLK_9:
                        current_effect = 8;
                        printf("Switched to Twister effect\n");
                        break;
                    case SDLK_m:
                        current_effect = 10;
                        printf("Switched to Metaballs effect\n");
                        break;
                    case SDLK_d:
                        current_effect = 11;
                        printf("Switched to Dot Tunnel effect\n");
                        break;
                    case SDLK_v:
                        current_effect = 12;
                        printf("Switched to Vector Balls effect\n");
                        break;
                    case SDLK_t:
                        current_effect = 13;
                        printf("Switched to Text Writer effect\n");
                        break;
                }
            }
        }

        // Update current effect
        frame_time = SDL_GetTicks();

        switch (current_effect) {
            case 0:
                plasma_update(pixels, frame_time);
                break;
            case 1:
                fire_update(pixels, frame_time);
                break;
            case 2:
                tunnel_update(pixels, frame_time);
                break;
            case 3:
                starfield_update(pixels, frame_time);
                break;
            case 4:
                scroller_update(pixels, frame_time);
                break;
            case 5:
                cube_update(pixels, frame_time);
                break;
            case 6:
                torus_update(pixels, frame_time);
                break;
            case 7:
                raster_update(pixels, frame_time);
                break;
            case 8:
                twister_update(pixels, frame_time);
                break;
            case 9:
                rotozoom_update(pixels, frame_time);
                break;
            case 10:
                metaballs_update(pixels, frame_time);
                break;
            case 11:
                dottunnel_update(pixels, frame_time);
                break;
            case 12:
                vectorballs_update(pixels, frame_time);
                break;
            case 13:
                textwriter_update(pixels, frame_time);
                break;
        }

        // Update texture and render
        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(pixel_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Cap frame rate (~60 FPS)
        SDL_Delay(16);
    }

    // Cleanup
    plasma_cleanup();
    fire_cleanup();
    tunnel_cleanup();
    starfield_cleanup();
    scroller_cleanup();
    cube_cleanup();
    torus_cleanup();
    raster_cleanup();
    twister_cleanup();
    rotozoom_cleanup();
    metaballs_cleanup();
    dottunnel_cleanup();
    vectorballs_cleanup();
    textwriter_cleanup();

    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
