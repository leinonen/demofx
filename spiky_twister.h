#ifndef SPIKY_TWISTER_H
#define SPIKY_TWISTER_H

#include "common.h"

void spiky_twister_init(void);
void spiky_twister_update(pixel_t *pixels, uint32_t time);
void spiky_twister_cleanup(void);

#endif
