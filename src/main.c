#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "epaper/epaper.h"
// WS2812 RGB LED driver
#include "ws2812.h"
#include "driver/ledc.h"
#include "wifi/wifi.h"

static const char *TAG = "main";

// WS2812 RGB LED pin definition
#define RGB_LED_PIN 8
#define LED_PIN 15

void app_main(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&io_conf);

    esp_log_level_set("*", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "INITIALIZING SYSTEM...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    for(int i = 0; i < 4; i++) {
        gpio_set_level(LED_PIN, 1); 
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
  /*
    uint16_t pin = 14;

gpio_config_t io_conf_pwd = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL << pin),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_DISABLE
};
gpio_config(&io_conf_pwd);
while(1) {
    gpio_set_level(pin, 1); 
    gpio_set_level(LED_PIN, 1); 
    vTaskDelay(pdMS_TO_TICKS(2000));
    gpio_set_level(pin, 0);
    gpio_set_level(LED_PIN, 0); 
    vTaskDelay(pdMS_TO_TICKS(2000));
}
*/
/*

uint16_t pin = 14;
uint16_t duty = 30;     // Percentage of duty cycle (0-100)

// Configure PWM for pin 14
ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .freq_hz = 100000, // 100 kHz
    .clk_cfg = LEDC_AUTO_CLK
};
ledc_timer_config(&ledc_timer);

ledc_channel_config_t ledc_channel = {
    .gpio_num = pin,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = (duty * 255) / 100, // Calculate duty based on percentage (max 255 for 8-bit)
    .hpoint = 0
};
ledc_channel_config(&ledc_channel);

while (1) {
    vTaskDelay(pdMS_TO_TICKS(10000));
}


return;*/
    
/*
    // Initialize WS2812 RGB LED
    ws2812_init(RGB_LED_PIN);
    ESP_LOGI(TAG, "WS2812 RGB LED initialized");
    // Set LED to red as example
    ws2812_set_color(255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    ws2812_set_color(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    ws2812_set_color(0, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(500));*/
wifi_mgr_init();
wifi_mgr_connect("Pixel_3683", "sk2gdgprw4w4mf4");
wifi_mgr_wait_for_connection(10000); // Wait up to 10 seconds

if (wifi_mgr_is_connected()) {
    char ip[16];
    wifi_mgr_get_ip_address(ip);
    ESP_LOGI(TAG, "Connected with IP: %s", ip);
}*/

    ESP_LOGI(TAG, "Starting E-Paper Display Example");

    // Initialize E-Paper display
    epaper_init();
    
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Set tri-color mode (0 = tri-color, 1 = B/W only)
    epaper_set_bw_mode(0);
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "E-Paper display initialized successfully");
    
    // Clear the display (set all pixels to white)
    epaper_clearDisplay();
    ESP_LOGI(TAG, "Display cleared");
    
    vTaskDelay(pdMS_TO_TICKS(20000));
    ESP_LOGI(TAG, "Draw Red");
    epaper_fill(COLOR_RED); // Start with red background
    vTaskDelay(pdMS_TO_TICKS(20000));
    ESP_LOGI(TAG, "Draw Black");
    epaper_fill(COLOR_BLACK); // Start with black background
    /*
    vTaskDelay(pdMS_TO_TICKS(20000));
    ESP_LOGI(TAG, "Draw Black");
    epaper_fill(COLOR_BLACK); // Start with black background
    vTaskDelay(pdMS_TO_TICKS(20000));
    ESP_LOGI(TAG, "Draw Red");
    epaper_fill(COLOR_RED); // Start with red background
*/
    /*
    // Draw a black border
    epaper_draw_rect(0, 0, config.width - 1, config.height - 1, COLOR_BLACK);
    
    // Draw black lines
    epaper_draw_line(10, 10, 100, 50, COLOR_BLACK);
    epaper_draw_line(10, 50, 100, 10, COLOR_BLACK);
    
    // Draw a red filled rectangle
    epaper_fill_rect(120, 10, 50, 30, COLOR_RED);
    
    // Draw a black filled rectangle
    epaper_fill_rect(120, 50, 50, 30, COLOR_BLACK);
    
    // Draw red lines
    epaper_draw_line(10, 70, 100, 110, COLOR_RED);
    
    // Update display with the buffer content
    epaper_update();
    ESP_LOGI(TAG, "Display updated");
    
    ESP_LOGI(TAG, "Example completed. Display will remain showing the pattern.");
    */
    // Enter deep sleep or continue with other tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
