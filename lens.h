#ifndef LENS_H
#define LENS_H

#include "common.h"

void lens_init(void);
void lens_update(pixel_t *pixels, uint32_t time);
void lens_cleanup(void);

#endif // LENS_H
