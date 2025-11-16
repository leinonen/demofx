#ifndef METABALLS_H
#define METABALLS_H

#include "common.h"

void metaballs_init(void);
void metaballs_update(pixel_t *pixels, uint32_t time);
void metaballs_cleanup(void);

#endif // METABALLS_H
