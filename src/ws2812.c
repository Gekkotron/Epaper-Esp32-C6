// WS2812 RGB LED driver for ESP32
// Simple implementation using RMT peripheral

#include "driver/rmt.h"
#include "esp_log.h"
#include "ws2812.h"

#define WS2812_RMT_CHANNEL RMT_CHANNEL_0
#define WS2812_LED_NUMBER 1
#define WS2812_STRIP_LENGTH WS2812_LED_NUMBER

static const char *TAG = "ws2812";

void ws2812_init(gpio_num_t gpio_num) {
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = WS2812_RMT_CHANNEL,
        .gpio_num = gpio_num,
        .clk_div = 2,
        .mem_block_num = 1,
        .tx_config = {
            .loop_en = false,
            .carrier_en = false,
            .idle_output_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,
        }
    };
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b) {
    rmt_item32_t items[24];
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    for (int i = 0; i < 24; i++) {
        uint32_t bit = (color >> (23 - i)) & 1;
        if (bit) {
            items[i].duration0 = 14;
            items[i].level0 = 1;
            items[i].duration1 = 7;
            items[i].level1 = 0;
        } else {
            items[i].duration0 = 7;
            items[i].level0 = 1;
            items[i].duration1 = 14;
            items[i].level1 = 0;
        }
    }
    rmt_write_items(WS2812_RMT_CHANNEL, items, 24, true);
    rmt_wait_tx_done(WS2812_RMT_CHANNEL, pdMS_TO_TICKS(100));
}
