#ifndef STARFIELD_H
#define STARFIELD_H

#include "common.h"

void starfield_init(void);
void starfield_update(pixel_t *pixels, uint32_t time);
void starfield_cleanup(void);

#endif // STARFIELD_H
