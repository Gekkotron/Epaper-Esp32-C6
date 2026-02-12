#ifndef EPAPER_UTILS_H
#define EPAPER_UTILS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    COLOR_WHITE = 0,
    COLOR_BLACK = 1,
    COLOR_RED = 2
} epaper_color_t;

#define __xdata

uint8_t epaper_color_bw(uint8_t color);
uint8_t epaper_color_red(uint8_t color);

#endif // EPAPER_UTILS_H