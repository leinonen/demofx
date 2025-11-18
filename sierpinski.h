#ifndef SIERPINSKI_H
#define SIERPINSKI_H

#include "common.h"

void sierpinski_init(void);
void sierpinski_update(pixel_t *pixels, uint32_t time);
void sierpinski_cleanup(void);

#endif // SIERPINSKI_H
