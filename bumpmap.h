#ifndef BUMPMAP_H
#define BUMPMAP_H

#include "common.h"

void bumpmap_init(void);
void bumpmap_update(pixel_t *pixels, uint32_t time);
void bumpmap_cleanup(void);

#endif
