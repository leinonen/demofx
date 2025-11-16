#ifndef TEXTWRITER_H
#define TEXTWRITER_H

#include "common.h"

void textwriter_init(void);
void textwriter_update(pixel_t *pixels, uint32_t time);
void textwriter_cleanup(void);

#endif // TEXTWRITER_H
