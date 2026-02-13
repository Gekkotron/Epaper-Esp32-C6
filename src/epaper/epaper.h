#ifndef EPAPER_H
#define EPAPER_H

#include "epaper_utils.h"
#include <stddef.h>
#include <stdint.h>

#define SCREEN_2_6_WIDTH 152
#define SCREEN_2_6_HEIGHT 296
#define BYTES_PER_ROW (SCREEN_2_6_WIDTH / 8)  // 19
#define BUFFER_SIZE (BYTES_PER_ROW * SCREEN_2_6_HEIGHT)

extern const uint8_t register_data[];

void epaper_reset(uint32_t ms1, uint32_t ms2, uint32_t ms3, uint32_t ms4, uint32_t ms5);
void epaper_sendIndexData(uint8_t index, const uint8_t *data, uint32_t len);
void epaper_softReset(void);
void epaper_waitBusy(void);

void epaper_send_command(uint8_t cmd);
void epaper_send_data(uint8_t data);
void epaper_send_color(uint8_t index, const uint8_t data, uint32_t len);

void epaper_clearDisplay(void);
void epaper_fill(uint8_t color);

void epaper_set_bw_mode(uint8_t bw_only);

void epaper_init(void);
void epaper_flushDisplay(void);

void epaper_DCDC_powerOn(void);
void epaper_DCDC_powerOff(void);

void epaper_send_buffer(const uint8_t *buffer, size_t length);

// Display pixel
void epaper_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color);
void epaper_set_partial_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void epaper_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);
void epaper_display_update(void); // Send framebuffer to display
void epaper_display_clear(void);  // Clear framebuffer
void epaper_test_partial_update(void); // Test if partial updates work

void test_rect();

// Screen orientation
#define ORIENTATION_0   0  // Normal (0°)
#define ORIENTATION_90  1  // Rotated 90° clockwise
#define ORIENTATION_180 2  // Rotated 180°
#define ORIENTATION_270 3  // Rotated 270° clockwise (90° counter-clockwise)

// Global orientation functions
void epaper_set_orientation(uint8_t orientation);
uint8_t epaper_get_orientation(void);

// Text rendering functions - all use global orientation set by epaper_set_orientation()
// 5x8 font (small, basic)
void epaper_draw_char(uint16_t x, uint16_t y, char c, uint8_t color, uint8_t scale);
void epaper_draw_text(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale);

// 6x12 font (medium, clean)
void epaper_draw_char_6x12(uint16_t x, uint16_t y, char c, uint8_t color, uint8_t scale);
void epaper_draw_text_6x12(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale);

// 8x16 font (large, cleaner)
void epaper_draw_char_8x16(uint16_t x, uint16_t y, char c, uint8_t color, uint8_t scale);
void epaper_draw_text_8x16(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale);

#endif // EPAPER_H
