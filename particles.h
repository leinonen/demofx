#ifndef PARTICLES_H
#define PARTICLES_H

#include "common.h"

void particles_init(void);
void particles_update(pixel_t *pixels, uint32_t time);
void particles_cleanup(void);

#endif // PARTICLES_H
