#include "epaper.h"
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_log.h"

// GPIO pin definitions - adjust these according to your wiring
#define PIN_NUM_MOSI    18          // Violet
#define PIN_NUM_CLK     19          // Bleu
#define PIN_NUM_CS      20          // Blanc
#define PIN_NUM_DC      0           // Jaune
#define PIN_NUM_RST     1           // Orange
#define PIN_NUM_BUSY    2           // Brun
#define PIN_NUM_PWR     14          // Vert


// Ecran lidl = 5.79 pouces 792 x 272

const uint8_t register_data[] = {0x00, 0x0e, 0x19, 0x02, 0xcf, 0x8d};

static spi_device_handle_t spi_device = NULL;

// Minimal SPI setup (call once before using epaper functions)
void epaper_spi_init(void) {
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 200 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_device);
}

void epaper_send_data(uint8_t data) {
    gpio_set_level(PIN_NUM_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(spi_device, &t);
}

void epaper_clearDisplay(void)
{
    epaper_sendColor(0x10, 0x00, (uint32_t)BUFFER_SIZE);
    epaper_sendColor(0x13, 0x00, (uint32_t)BUFFER_SIZE);
    epaper_flushDisplay();
}

void epaper_flushDisplay(void)
{
    epaper_DCDC_powerOn();
    epaper_send_command(0x12);
    epaper_waitBusy();
    epaper_DCDC_powerOff();
}

void epaper_DCDC_powerOn(void)
{
  epaper_sendIndexData(0x04, &register_data[0], 1);
  epaper_waitBusy();
}

void epaper_DCDC_powerOff(void)
{
  epaper_sendIndexData(0x02, &register_data[0], 0);
  epaper_waitBusy();
}

void epaper_sendColor(uint8_t index, const uint8_t data, uint32_t len)
{
    epaper_send_command(index);
    gpio_set_level(PIN_NUM_DC, 1); // Data mode
    gpio_set_level(PIN_NUM_CS, 0); // Select
    // Send the same byte 'data' for 'len' times
    for (uint32_t i = 0; i < len; i++) {
        spi_transaction_t t = {
            .length = 8, // 1 byte = 8 bits
            .tx_buffer = &data,
        };
        spi_device_polling_transmit(spi_device, &t);
        // Yield every 1024 bytes to avoid watchdog reset
        if ((i % 1024) == 0) {
            vTaskDelay(1);
        }
    }
    gpio_set_level(PIN_NUM_CS, 1); // Deselect
}

void epaper_fill(uint8_t color)
{
    uint8_t bw = 0x00, red = 0x00;
    if (color == COLOR_WHITE) { // COLOR_WHITE
        bw = 0x00; red = 0x00;
    } else if (color == COLOR_BLACK) { // COLOR_BLACK
        bw = 0x00; red = 0xFF;
    } else if (color == COLOR_RED) { // COLOR_RED
        bw = 0xFF; red = 0xFF;
    }
    // Fill BW channel (0x13) FIRST
    epaper_send_command(0x13);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        epaper_send_data(bw);
    }
    // Fill RED channel (0x10) SECOND
    epaper_send_command(0x10);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        epaper_send_data(red);
    }
    // Refresh display (stub)
    epaper_send_command(0x12);
}

void epaper_set_bw_mode(uint8_t bw_only)
{
    uint8_t psr_data[2];
    // Example PSR defaults for 2.13" display
    psr_data[0] = register_data[4];
    psr_data[1] = register_data[5];
    if (bw_only) {
        psr_data[0] |= 0x10; // Set bit 4 for B/W mode
    } else {
        psr_data[0] &= ~0x10; // Clear bit 4 for tri-color mode
    }
    epaper_sendIndexData(0x00, psr_data, 2);
}

void epaper_init()
{
    // Configure DC, RST, BUSY pins as GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_PWR),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&io_conf);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_NUM_BUSY);
    gpio_config(&io_conf);

    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for power to stabilize

    // Initialize SPI (call only once)
    epaper_spi_init();

    gpio_set_level(PIN_NUM_PWR, 0); // Enable power (active low)
    ESP_LOGI("epaper", "EPD power enabled (active low)");

    vTaskDelay(pdMS_TO_TICKS(5)); // Short delay before reset

    // Hardware reset sequence
    epaper_reset(1, 5, 10, 5, 1);

    // Wait for display to be ready
    epaper_waitBusy();

    // Software reset
    epaper_softReset();

    epaper_sendIndexData(0xe5, &register_data[2], 1); // Input Temperature: 25C
    epaper_sendIndexData(0xe0, &register_data[3], 1); // Active Temperature
    epaper_sendIndexData(0x00, &register_data[4], 2); // PSR

    ESP_LOGI("epaper", "EPD initialization complete");
}

void epaper_reset(uint32_t ms1, uint32_t ms2, uint32_t ms3, uint32_t ms4, uint32_t ms5)
{
  vTaskDelay(pdMS_TO_TICKS(ms1));
  gpio_set_level(PIN_NUM_RST, 0); // RST low
  vTaskDelay(pdMS_TO_TICKS(ms2));
  gpio_set_level(PIN_NUM_RST, 1); // RST high
  vTaskDelay(pdMS_TO_TICKS(ms3));
  gpio_set_level(PIN_NUM_RST, 0); // RST low
  vTaskDelay(pdMS_TO_TICKS(ms4));
  gpio_set_level(PIN_NUM_RST, 1); // RST high
  vTaskDelay(pdMS_TO_TICKS(ms5));
}

void epaper_waitBusy(void)
{
    int timeout = 5000; // 5000 x 2ms = 10s max wait
    // NOTE: Some displays use BUSY=1 when busy, others BUSY=0. Adjust logic if needed!
    do {
        int busy_level = gpio_get_level(PIN_NUM_BUSY);
        ESP_LOGI("epaper", "BUSY pin state: %d", busy_level);
        vTaskDelay(pdMS_TO_TICKS(2));
        timeout--;
        if (timeout <= 0) {
            ESP_LOGE("epaper", "BUSY pin timeout!");
            break;
        }
    } while (gpio_get_level(PIN_NUM_BUSY) == 1);
    vTaskDelay(pdMS_TO_TICKS(200));
}

void epaper_softReset(void)
{
  epaper_sendIndexData(0x00, &register_data[1], 1);
  epaper_waitBusy();
}

void epaper_sendIndexData(uint8_t index, const uint8_t *data, uint32_t len)
{
    epaper_send_command(index);
    gpio_set_level(PIN_NUM_DC, 1); // Data mode
    gpio_set_level(PIN_NUM_CS, 0); // Select
    if (len > 0) {
        spi_transaction_t t = {
            .length = len * 8, // length in bits
            .tx_buffer = data,
        };
        esp_err_t error = spi_device_polling_transmit(spi_device, &t);
        if (error != ESP_OK) {
            ESP_LOGE("epaper", "SPI transmit error: %d", error);
        }
    }
    gpio_set_level(PIN_NUM_CS, 1); // Deselect
}

void epaper_send_command(uint8_t cmd)
{
    gpio_set_level(PIN_NUM_DC, 0); // Command mode
    gpio_set_level(PIN_NUM_CS, 0); // Select
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    esp_err_t error = spi_device_polling_transmit(spi_device, &t);
    if (error != ESP_OK) {
        ESP_LOGE("epaper", "SPI transmit error: %d", error);
    }
  gpio_set_level(PIN_NUM_CS, 1); // Deselect
}

#define MAX_SPI_CHUNK 4096
void epaper_send_buffer(const uint8_t *buffer, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        size_t chunk = (length - offset > MAX_SPI_CHUNK) ? MAX_SPI_CHUNK : (length - offset);
        spi_transaction_t t = {
            .length = chunk * 8, // bits
            .tx_buffer = buffer + offset,
        };
        esp_err_t ret = spi_device_polling_transmit(spi_device, &t);
        if (ret != ESP_OK) {
            ESP_LOGE("epaper", "SPI transmit error: %d", ret);
            break;
        }
        offset += chunk;
    }
}