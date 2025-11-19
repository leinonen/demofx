#ifndef MATRIXCODE_H
#define MATRIXCODE_H

#include "common.h"

void matrixcode_init(void);
void matrixcode_update(pixel_t *pixels, uint32_t time);
void matrixcode_cleanup(void);

#endif // MATRIXCODE_H
