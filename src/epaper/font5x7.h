#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

// Simple 5x8 bitmap font for ASCII characters 32-126
// Each character is 5 bytes (5 columns x 8 rows)
// Bit 0 = top, Bit 7 = bottom (used for descenders like y, g, p, q, j)

extern const uint8_t font5x7[][5];

#define FONT_WIDTH  5
#define FONT_HEIGHT 8
#define FONT_FIRST_CHAR 32   // Space
#define FONT_LAST_CHAR  129  // Extended characters (127=°, 128=é, 129=è)

#endif // FONT5X7_H
