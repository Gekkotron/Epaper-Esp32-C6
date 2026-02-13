#ifndef FONT8X16_H
#define FONT8X16_H

#include <stdint.h>

// 8x16 bitmap font for ASCII characters 32-126
// Each character is 16 bytes (8 columns x 16 rows)
// Each byte represents one row of 8 pixels

extern const uint8_t font8x16[][16];

#define FONT8X16_WIDTH  8
#define FONT8X16_HEIGHT 16
#define FONT8X16_FIRST_CHAR 32   // Space
#define FONT8X16_LAST_CHAR  129  // Extended characters (127=°, 128=é, 129=è)

#endif // FONT8X16_H
