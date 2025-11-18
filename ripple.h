#ifndef RIPPLE_H
#define RIPPLE_H

#include <stdint.h>

typedef uint32_t pixel_t;

void ripple_init(void);
void ripple_update(pixel_t *pixels, uint32_t time);
void ripple_cleanup(void);

#endif
