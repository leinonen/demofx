#ifndef TWISTER_H
#define TWISTER_H

#include "common.h"

void twister_init(void);
void twister_update(pixel_t *pixels, uint32_t time);
void twister_cleanup(void);

#endif // TWISTER_H
