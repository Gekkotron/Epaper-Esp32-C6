#ifndef EPAPER_H
#define EPAPER_H

#include "epaper_utils.h"
#include <stddef.h>
#include <stdint.h>

#define SCREEN_2_6_WIDTH 152
#define SCREEN_2_6_HEIGHT 296
#define BUFFER_SIZE 44992

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

#endif // EPAPER_H
