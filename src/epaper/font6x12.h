#ifndef FONT6X12_H
#define FONT6X12_H

#include <stdint.h>

// 6x12 bitmap font for ASCII characters 32-126
// Each character is 12 bytes (12 rows of 6 pixels)
// Good balance between readability and space

extern const uint8_t font6x12[][12];

#define FONT6X12_WIDTH  6
#define FONT6X12_HEIGHT 12
#define FONT6X12_FIRST_CHAR 32   // Space
#define FONT6X12_LAST_CHAR  129  // Extended characters (127=°, 128=é, 129=è)

#endif // FONT6X12_H
