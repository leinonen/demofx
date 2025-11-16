#ifndef PLASMA_H
#define PLASMA_H

#include "common.h"

void plasma_init(void);
void plasma_update(pixel_t *pixels, uint32_t time);
void plasma_cleanup(void);

#endif // PLASMA_H
