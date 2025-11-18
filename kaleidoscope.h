#ifndef KALEIDOSCOPE_H
#define KALEIDOSCOPE_H

#include "common.h"

void kaleidoscope_init(void);
void kaleidoscope_update(pixel_t *pixels, uint32_t time);
void kaleidoscope_cleanup(void);

#endif
