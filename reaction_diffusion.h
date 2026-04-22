#ifndef REACTION_DIFFUSION_H
#define REACTION_DIFFUSION_H

#include "common.h"

void reaction_diffusion_init(void);
void reaction_diffusion_update(pixel_t *pixels, uint32_t time);
void reaction_diffusion_cleanup(void);

#endif // REACTION_DIFFUSION_H
