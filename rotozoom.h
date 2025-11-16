#ifndef ROTOZOOM_H
#define ROTOZOOM_H

#include "common.h"

void rotozoom_init(void);
void rotozoom_update(pixel_t *pixels, uint32_t time);
void rotozoom_cleanup(void);

#endif // ROTOZOOM_H
