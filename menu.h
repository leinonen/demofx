#ifndef MENU_H
#define MENU_H

#include "common.h"

// Initialize menu system with effect names
void menu_init(const char **names, int count);

// Draw menu overlay on top of background pixels
// selected_index: currently highlighted effect (0-26)
void menu_draw(pixel_t *background_pixels, int selected_index);

// Cleanup menu resources
void menu_cleanup(void);

// Get total number of effects
int menu_get_effect_count(void);

// Get effect name by index
const char* menu_get_effect_name(int index);

#endif // MENU_H
