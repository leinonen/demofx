#ifndef PARALLAX_H
#define PARALLAX_H

#include "common.h"

void parallax_init(void);
void parallax_update(pixel_t *pixels, uint32_t time);
void parallax_cleanup(void);

#endif // PARALLAX_H
