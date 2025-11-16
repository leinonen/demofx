#ifndef FIRE_H
#define FIRE_H

#include "common.h"

void fire_init(void);
void fire_update(pixel_t *pixels, uint32_t time);
void fire_cleanup(void);

#endif // FIRE_H
