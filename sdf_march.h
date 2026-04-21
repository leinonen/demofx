#ifndef SDF_MARCH_H
#define SDF_MARCH_H

#include "common.h"

void sdf_march_init(void);
void sdf_march_update(pixel_t *pixels, uint32_t time);
void sdf_march_cleanup(void);

#endif
