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
#include "webserver/webserver.h"
#include "config/config.h"

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

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  E-PAPER DISPLAY READY!");
    ESP_LOGI(TAG, "========================================");

    // Load configuration from .env file
    ESP_LOGI(TAG, "Loading configuration...");
    config_init();

    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_mgr_init();

    const char *ssid = config_get_wifi_ssid();
    const char *password = config_get_wifi_password();

    // Fallback to hardcoded credentials if .env is not available
    if (strlen(ssid) == 0 || strlen(password) == 0) {
        ESP_LOGE(TAG, "No WiFi credentials found in .env file!");
        ESP_LOGW(TAG, "Please create data/.env with WIFI_SSID and WIFI_PASSWORD");
        ESP_LOGW(TAG, "Then run 'pio run --target uploadfs' to upload credentials");

        // Show error on screen
        epaper_display_clear();
        epaper_draw_text(10, 10, "Config Error", COLOR_RED, 2);
        epaper_draw_text(10, 35, "Missing .env", COLOR_BLACK, 1);
        epaper_draw_text(10, 50, "file", COLOR_BLACK, 1);
        epaper_display_update();

        // Keep running but skip WiFi
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }

    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    wifi_mgr_connect(ssid, password);
    wifi_mgr_wait_for_connection(10000);

    if (wifi_mgr_is_connected()) {
        char ip[16];
        wifi_mgr_get_ip_address(ip);
        ESP_LOGI(TAG, "✓ WiFi connected!");
        ESP_LOGI(TAG, "✓ IP address: %s", ip);

        // Display welcome message on screen
        epaper_display_clear();
        epaper_draw_text(10, 10, "E-Paper API", COLOR_BLACK, 2);
        epaper_draw_text(10, 35, "Ready!", COLOR_RED, 2);
        epaper_draw_text(10, 60, "IP:", COLOR_BLACK, 1);
        epaper_draw_text(30, 60, ip, COLOR_BLACK, 1);
        epaper_draw_text(10, 75, "Port: 80", COLOR_BLACK, 1);
        epaper_display_update();

        // Start web server
        ESP_LOGI(TAG, "Starting web server...");
        webserver_start();
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "  WEB SERVER RUNNING");
        ESP_LOGI(TAG, "  Open browser: http://%s", ip);
        ESP_LOGI(TAG, "========================================");
    } else {
        ESP_LOGE(TAG, "✗ WiFi connection failed");

        // Show error on screen
        epaper_display_clear();
        epaper_draw_text(10, 10, "WiFi Error", COLOR_RED, 2);
        epaper_draw_text(10, 35, "Check SSID", COLOR_BLACK, 1);
        epaper_display_update();
    }

    // Keep running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
