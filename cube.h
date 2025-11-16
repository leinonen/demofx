#ifndef CUBE_H
#define CUBE_H

#include "common.h"

void cube_init(void);
void cube_update(pixel_t *pixels, uint32_t time);
void cube_cleanup(void);

#endif // CUBE_H
