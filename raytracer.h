#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "common.h"

void raytracer_init(void);
void raytracer_update(pixel_t *pixels, uint32_t time);
void raytracer_cleanup(void);

#endif
