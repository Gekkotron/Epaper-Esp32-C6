#ifndef EPAPER_H
#define EPAPER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    COLOR_WHITE = 0,
    COLOR_BLACK = 1,
    COLOR_RED = 2
} epaper_color_t;

#define BUFFER_SIZE 50000 // Example for 2.13" display, update as needed

extern const uint8_t register_data[];

void epaper_reset(uint32_t ms1, uint32_t ms2, uint32_t ms3, uint32_t ms4, uint32_t ms5);
void epaper_sendIndexData(uint8_t index, const uint8_t *data, uint32_t len);
void epaper_softReset(void);
void epaper_waitBusy(void);

void epaper_send_command(uint8_t cmd);
void epaper_send_data(uint8_t data);
void epaper_sendColor(uint8_t index, const uint8_t data, uint32_t len);

void epaper_clearDisplay(void);
void epaper_fill(uint8_t color);

void epaper_set_bw_mode(uint8_t bw_only);

void epaper_init(void);
void epaper_flushDisplay(void);

void epaper_DCDC_powerOn(void);
void epaper_DCDC_powerOff(void);

void epaper_send_buffer(const uint8_t *buffer, size_t length);

#endif // EPAPER_H
