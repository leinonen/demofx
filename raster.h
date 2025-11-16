#ifndef RASTER_H
#define RASTER_H

#include "common.h"

void raster_init(void);
void raster_update(pixel_t *pixels, uint32_t time);
void raster_cleanup(void);

#endif // RASTER_H
