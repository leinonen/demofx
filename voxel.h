#ifndef VOXEL_H
#define VOXEL_H

#include <stdint.h>

typedef uint32_t pixel_t;

void voxel_init(void);
void voxel_update(pixel_t *pixels, uint32_t time);
void voxel_cleanup(void);

#endif
