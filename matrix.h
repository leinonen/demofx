#ifndef MATRIX_H
#define MATRIX_H

#include "common.h"

void matrix_init(void);
void matrix_update(pixel_t *pixels, uint32_t time);
void matrix_cleanup(void);

#endif // MATRIX_H
