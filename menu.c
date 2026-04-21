#include "menu.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#define MENU_WIDTH 200
#define MENU_HEIGHT 180
#define MENU_X ((SCREEN_WIDTH - MENU_WIDTH) / 2)
#define MENU_Y ((SCREEN_HEIGHT - MENU_HEIGHT) / 2)
#define MENU_PADDING 8
#define TITLE_HEIGHT 16
#define ITEM_HEIGHT 8
#define VISIBLE_ITEMS 15
#define SCANLINE_SPACING 2

// Retro futuristic color palette
#define COLOR_CYAN RGB(0, 255, 255)
#define COLOR_MAGENTA RGB(255, 0, 255)
#define COLOR_PURPLE RGB(180, 0, 255)
#define COLOR_DARK_CYAN RGB(0, 100, 120)
#define COLOR_DARK_PURPLE RGB(40, 0, 60)
#define COLOR_BLACK RGB(0, 0, 0)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_GRAY RGB(100, 100, 100)

// Effect names and count (provided by main)
static const char **effect_names = NULL;
static int effect_count = 0;

void menu_init(const char **names, int count) {
    effect_names = names;
    effect_count = count;
}

// Draw a horizontal line
static void draw_hline(pixel_t *pixels, int x, int y, int w, pixel_t color) {
    for (int i = 0; i < w; i++) {
        int px = x + i;
        if (px >= 0 && px < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            pixels[y * SCREEN_WIDTH + px] = color;
        }
    }
}

// Draw a vertical line
static void draw_vline(pixel_t *pixels, int x, int y, int h, pixel_t color) {
    for (int i = 0; i < h; i++) {
        int py = y + i;
        if (x >= 0 && x < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
            pixels[py * SCREEN_WIDTH + x] = color;
        }
    }
}

// Draw a filled rectangle
static void draw_filled_rect(pixel_t *pixels, int x, int y, int w, int h, pixel_t color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                pixels[py * SCREEN_WIDTH + px] = color;
            }
        }
    }
}

// Darken background pixels for menu visibility
static void darken_background(pixel_t *pixels) {
    for (int py = MENU_Y; py < MENU_Y + MENU_HEIGHT; py++) {
        for (int px = MENU_X; px < MENU_X + MENU_WIDTH; px++) {
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                int idx = py * SCREEN_WIDTH + px;
                pixel_t p = pixels[idx];
                // Extract RGB and darken to 25%
                int r = ((p >> 24) & 0xFF) / 4;
                int g = ((p >> 16) & 0xFF) / 4;
                int b = ((p >> 8) & 0xFF) / 4;
                pixels[idx] = RGB(r, g, b);
            }
        }
    }
}

// Draw retro grid border
static void draw_grid_border(pixel_t *pixels) {
    // Main border - double line for cyberpunk look
    draw_hline(pixels, MENU_X, MENU_Y, MENU_WIDTH, COLOR_CYAN);
    draw_hline(pixels, MENU_X, MENU_Y + 1, MENU_WIDTH, COLOR_CYAN);
    draw_hline(pixels, MENU_X, MENU_Y + MENU_HEIGHT - 2, MENU_WIDTH, COLOR_CYAN);
    draw_hline(pixels, MENU_X, MENU_Y + MENU_HEIGHT - 1, MENU_WIDTH, COLOR_CYAN);

    draw_vline(pixels, MENU_X, MENU_Y, MENU_HEIGHT, COLOR_CYAN);
    draw_vline(pixels, MENU_X + 1, MENU_Y, MENU_HEIGHT, COLOR_CYAN);
    draw_vline(pixels, MENU_X + MENU_WIDTH - 2, MENU_Y, MENU_HEIGHT, COLOR_CYAN);
    draw_vline(pixels, MENU_X + MENU_WIDTH - 1, MENU_Y, MENU_HEIGHT, COLOR_CYAN);

    // Corner accents - magenta highlights
    for (int i = 0; i < 6; i++) {
        // Top-left
        draw_hline(pixels, MENU_X + 2, MENU_Y + 2 + i, 6, COLOR_MAGENTA);
        draw_vline(pixels, MENU_X + 2 + i, MENU_Y + 2, 6, COLOR_MAGENTA);

        // Top-right
        draw_hline(pixels, MENU_X + MENU_WIDTH - 8, MENU_Y + 2 + i, 6, COLOR_MAGENTA);
        draw_vline(pixels, MENU_X + MENU_WIDTH - 3 - i, MENU_Y + 2, 6, COLOR_MAGENTA);

        // Bottom-left
        draw_hline(pixels, MENU_X + 2, MENU_Y + MENU_HEIGHT - 3 - i, 6, COLOR_MAGENTA);
        draw_vline(pixels, MENU_X + 2 + i, MENU_Y + MENU_HEIGHT - 8, 6, COLOR_MAGENTA);

        // Bottom-right
        draw_hline(pixels, MENU_X + MENU_WIDTH - 8, MENU_Y + MENU_HEIGHT - 3 - i, 6, COLOR_MAGENTA);
        draw_vline(pixels, MENU_X + MENU_WIDTH - 3 - i, MENU_Y + MENU_HEIGHT - 8, 6, COLOR_MAGENTA);
    }

    // Title separator line
    draw_hline(pixels, MENU_X + 4, MENU_Y + TITLE_HEIGHT + 6, MENU_WIDTH - 8, COLOR_PURPLE);
    draw_hline(pixels, MENU_X + 4, MENU_Y + TITLE_HEIGHT + 7, MENU_WIDTH - 8, COLOR_DARK_PURPLE);
}

// Draw scanlines for retro CRT effect
static void draw_scanlines(pixel_t *pixels) {
    for (int py = MENU_Y + 2; py < MENU_Y + MENU_HEIGHT - 2; py += SCANLINE_SPACING) {
        for (int px = MENU_X + 2; px < MENU_X + MENU_WIDTH - 2; px++) {
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                int idx = py * SCREEN_WIDTH + px;
                pixel_t p = pixels[idx];
                // Darken slightly for scanline
                int r = ((p >> 24) & 0xFF) * 9 / 10;
                int g = ((p >> 16) & 0xFF) * 9 / 10;
                int b = ((p >> 8) & 0xFF) * 9 / 10;
                pixels[idx] = RGB(r, g, b);
            }
        }
    }
}

void menu_draw(pixel_t *background_pixels, int selected_index) {
    // Darken background behind menu
    darken_background(background_pixels);

    // Draw grid border
    draw_grid_border(background_pixels);

    // Draw title with glow
    const char* title = "DEMOFX SELECTOR";
    int title_x = MENU_X + (MENU_WIDTH - (strlen(title) * 8)) / 2;
    int title_y = MENU_Y + 4;
    draw_text_with_glow(background_pixels, title_x, title_y, title, COLOR_CYAN, COLOR_DARK_CYAN);

    // Calculate scroll offset
    int scroll_offset = 0;
    if (selected_index >= VISIBLE_ITEMS) {
        scroll_offset = selected_index - VISIBLE_ITEMS + 1;
    }

    // Draw effect list
    int list_y = MENU_Y + TITLE_HEIGHT + 10;
    for (int i = 0; i < effect_count && i < VISIBLE_ITEMS + scroll_offset; i++) {
        if (i < scroll_offset) continue;

        int item_y = list_y + (i - scroll_offset) * (ITEM_HEIGHT + 1);
        int item_x = MENU_X + MENU_PADDING;

        if (i == selected_index) {
            // Draw selection highlight bar
            draw_filled_rect(background_pixels, MENU_X + 4, item_y - 1, MENU_WIDTH - 8, ITEM_HEIGHT + 1, COLOR_DARK_PURPLE);

            // Draw selection indicator
            draw_text(background_pixels, item_x, item_y, ">", COLOR_MAGENTA);

            // Draw effect name with cyan glow
            char numbered_name[64];
            snprintf(numbered_name, sizeof(numbered_name), "%2d %s", i + 1, effect_names[i]);
            draw_text_with_glow(background_pixels, item_x + 10, item_y, numbered_name, COLOR_CYAN, COLOR_DARK_CYAN);
        } else {
            // Draw unselected effect name
            char numbered_name[64];
            snprintf(numbered_name, sizeof(numbered_name), "%2d %s", i + 1, effect_names[i]);
            draw_text(background_pixels, item_x + 10, item_y, numbered_name, COLOR_PURPLE);
        }
    }

    // Draw scroll indicator if needed
    if (scroll_offset > 0) {
        draw_text(background_pixels, MENU_X + MENU_WIDTH - 16, list_y - 2, "^", COLOR_CYAN);
    }
    if (selected_index < effect_count - 1 && scroll_offset + VISIBLE_ITEMS < effect_count) {
        draw_text(background_pixels, MENU_X + MENU_WIDTH - 16, list_y + VISIBLE_ITEMS * (ITEM_HEIGHT + 1) - 6, "v", COLOR_CYAN);
    }

    // Draw controls hint at bottom
    const char* hint = "ENTER=SELECT ESC=QUIT";
    int hint_x = MENU_X + (MENU_WIDTH - (strlen(hint) * 8)) / 2;
    int hint_y = MENU_Y + MENU_HEIGHT - 12;
    draw_text(background_pixels, hint_x, hint_y, hint, COLOR_GRAY);

    // Apply scanlines for retro CRT effect
    draw_scanlines(background_pixels);
}

void menu_cleanup(void) {
    // Nothing to cleanup for now
}

int menu_get_effect_count(void) {
    return effect_count;
}

const char* menu_get_effect_name(int index) {
    if (index >= 0 && index < effect_count && effect_names != NULL) {
        return effect_names[index];
    }
    return "Unknown";
}
