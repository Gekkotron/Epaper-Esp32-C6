#include "epaper_utils.h"

#include <stddef.h>
#include <stdint.h>

uint8_t epaper_color_bw(uint8_t color) {
    // Returns data for 0x13 channel (BW channel)
    // 0xFF = activate black, 0x00 = white/clear
    if (color == COLOR_WHITE) {
        return 0x00; // No black
    } else if (color == COLOR_BLACK) {
        return 0xFF; // Activate black
    } else if (color == COLOR_RED) {
        return 0x00; // No black (red will be on red channel)
    }
    return 0x00;
}

uint8_t epaper_color_red(uint8_t color) {
    // Returns data for 0x10 channel (RED channel)
    // 0xFF = activate red, 0x00 = no red
    if (color == COLOR_WHITE) {
        return 0x00; // No red
    } else if (color == COLOR_BLACK) {
        return 0x00; // No red (black is on BW channel)
    } else if (color == COLOR_RED) {
        return 0xFF; // Activate red
    }
    return 0x00;
}