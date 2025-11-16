#ifndef TUNNEL_H
#define TUNNEL_H

#include "common.h"

void tunnel_init(void);
void tunnel_update(pixel_t *pixels, uint32_t time);
void tunnel_cleanup(void);

#endif // TUNNEL_H
