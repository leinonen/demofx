#ifndef DOTTUNNEL_H
#define DOTTUNNEL_H

#include "common.h"

void dottunnel_init(void);
void dottunnel_update(pixel_t *pixels, uint32_t time);
void dottunnel_cleanup(void);

#endif // DOTTUNNEL_H
