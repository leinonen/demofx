#ifndef TESSERACT_H
#define TESSERACT_H

#include "common.h"

void tesseract_init(void);
void tesseract_update(pixel_t *pixels, uint32_t time);
void tesseract_cleanup(void);

#endif
