#include "epaper_utils.h"

#include <stddef.h>
#include <stdint.h>

uint8_t epaper_color_bw(uint8_t color) {
    uint8_t bw = 0x00;
    if (color == COLOR_WHITE) { // COLOR_WHITE
        bw = 0x00;
    } else if (color == COLOR_BLACK) { // COLOR_BLACK
        bw = 0x00;
    } else if (color == COLOR_RED) { // COLOR_RED
        bw = 0xFF;
    }

    return bw;
}

uint8_t epaper_color_red(uint8_t color) {
    uint8_t red = 0x00;
        if (color == COLOR_WHITE) { // COLOR_WHITE
            red = 0x00;
        } else if (color == COLOR_BLACK) { // COLOR_BLACK
            red = 0xFF;
        } else if (color == COLOR_RED) { // COLOR_RED
            red = 0xFF;
        }
    
        return red;
    }