#ifndef TORUS_H
#define TORUS_H

#include "common.h"

void torus_init(void);
void torus_update(pixel_t *pixels, uint32_t time);
void torus_cleanup(void);

#endif // TORUS_H
