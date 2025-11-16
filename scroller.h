#ifndef SCROLLER_H
#define SCROLLER_H

#include "common.h"

void scroller_init(void);
void scroller_update(pixel_t *pixels, uint32_t time);
void scroller_cleanup(void);

#endif // SCROLLER_H
