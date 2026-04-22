#ifndef MORPH3D_H
#define MORPH3D_H
#include "common.h"
void morph3d_init(void);
void morph3d_update(pixel_t *pixels, uint32_t time);
void morph3d_cleanup(void);
#endif
