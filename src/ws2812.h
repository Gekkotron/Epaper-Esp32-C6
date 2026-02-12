#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include "driver/gpio.h"

void ws2812_init(gpio_num_t gpio_num);
void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif // WS2812_H
