#ifndef VORONOI_H
#define VORONOI_H
#include "common.h"

void voronoi_init(void);
void voronoi_update(pixel_t *pixels, uint32_t time);
void voronoi_cleanup(void);

#endif
