#include "epaper.h"
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_log.h"
#include "epaper_utils.h"

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
        .clock_speed_hz = 4 * 1000 * 1000,
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
    epaper_send_color(0x10, 0x00, (uint32_t)BUFFER_SIZE);
    epaper_send_color(0x13, 0x00, (uint32_t)BUFFER_SIZE);
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

void epaper_send_color(uint8_t index, const uint8_t data, uint32_t len)
{
    epaper_send_command(index);
    gpio_set_level(PIN_NUM_DC, 1); // Data mode
    gpio_set_level(PIN_NUM_CS, 0); // Select
    // Send the same byte 'data' for 'len' times
    for (uint32_t i = 0; i < len / 8; i++) {
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
    uint8_t bw = epaper_color_bw(color), red = epaper_color_red(color);

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

void epaper_set_partial_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  // Align coordinates to byte boundaries (multiples of 8)
  uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
  uint16_t ye = y + h - 1;
  x &= 0xFFF8; // byte boundary

  // Send partial window command with correct byte order (GxEPD2 format)
  uint8_t params[7];
  params[0] = x % 256;        // x LSB
  params[1] = xe % 256;       // xe LSB
  params[2] = y / 256;        // y MSB
  params[3] = y % 256;        // y LSB
  params[4] = ye / 256;       // ye MSB
  params[5] = ye % 256;       // ye LSB
  params[6] = 0x01;           // Control byte

  epaper_sendIndexData(0x90, params, 7);
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

        // Yield to watchdog every 8KB to prevent timeout
        if (offset % 8192 == 0) {
            vTaskDelay(1);
        }
    }
}

void epaper_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color) {
    epaper_DCDC_powerOn();

    uint8_t bw = epaper_color_bw(color), red = epaper_color_red(color);
    epaper_send_color(0x10, bw, BUFFER_SIZE);
    epaper_send_color(0x13, red, BUFFER_SIZE);

    epaper_send_command(0x12);

    epaper_waitBusy();
    epaper_DCDC_powerOff();
}


// Static framebuffers for display content
static uint8_t *framebuffer_bw = NULL;
static uint8_t *framebuffer_red = NULL;

// Global screen orientation (default: 0°)
static uint8_t screen_orientation = ORIENTATION_0;

// Forward declaration for coordinate transformation
static inline void transform_coordinates(uint16_t x, uint16_t y, uint8_t orientation, uint16_t *out_x, uint16_t *out_y);

// Initialize framebuffers (call once)
static void epaper_framebuffer_init(void) {
    if (framebuffer_bw == NULL) {
        framebuffer_bw = (uint8_t*)malloc(BUFFER_SIZE);
        if (framebuffer_bw == NULL) {
            ESP_LOGE("epaper", "Failed to allocate BW framebuffer");
            return;
        }
        memset(framebuffer_bw, 0x00, BUFFER_SIZE);
        ESP_LOGI("epaper", "Allocated BW framebuffer: %d bytes", BUFFER_SIZE);
    }

    if (framebuffer_red == NULL) {
        framebuffer_red = (uint8_t*)malloc(BUFFER_SIZE);
        if (framebuffer_red == NULL) {
            ESP_LOGE("epaper", "Failed to allocate RED framebuffer");
            return;
        }
        memset(framebuffer_red, 0x00, BUFFER_SIZE);
        ESP_LOGI("epaper", "Allocated RED framebuffer: %d bytes", BUFFER_SIZE);
    }
}

// Helper function: draw a single pixel directly without orientation (for internal use)
static inline void epaper_draw_pixel_direct(uint16_t x, uint16_t y, uint8_t color) {
    // Check bounds
    if (x >= SCREEN_2_6_WIDTH || y >= SCREEN_2_6_HEIGHT) {
        return;
    }

    // Calculate framebuffer position
    uint16_t byte_idx = y * BYTES_PER_ROW + (x / 8);
    uint8_t bit_mask = 0x80 >> (x % 8);

    uint8_t bw = epaper_color_bw(color);
    uint8_t red = epaper_color_red(color);

    if (bw) {
        framebuffer_bw[byte_idx] |= bit_mask;
    } else {
        framebuffer_bw[byte_idx] &= ~bit_mask;
    }

    if (red) {
        framebuffer_red[byte_idx] |= bit_mask;
    } else {
        framebuffer_red[byte_idx] &= ~bit_mask;
    }
}

// Helper function: draw a single pixel with orientation support
static inline void epaper_draw_pixel(uint16_t x, uint16_t y, uint8_t color) {
    // Apply orientation transformation
    uint16_t out_x, out_y;
    transform_coordinates(x, y, screen_orientation, &out_x, &out_y);
    epaper_draw_pixel_direct(out_x, out_y, color);
}

// Draw rectangle (uses global orientation)
void epaper_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color) {
    epaper_framebuffer_init();

    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            epaper_draw_pixel(x + col, y + row, color);
        }
    }
}

void test_rect() {
    epaper_display_clear();
    
    // Rectangle aligné sur byte boundary (plus facile à débugger)
    epaper_rect(0, 0, 32, 32, COLOR_BLACK);   // coin haut-gauche
    epaper_rect(40, 0, 32, 32, COLOR_RED);    // à côté
    
    // Debug
    ESP_LOGI("epaper", "Byte 0: 0x%02X (should be 0xFF if black)", framebuffer_bw[0]);
    ESP_LOGI("epaper", "Byte 19: 0x%02X (row 1, should be 0xFF)", framebuffer_bw[BYTES_PER_ROW]);
    
    epaper_display_update();
}

// Send framebuffer to display and refresh (call after drawing operations)
void epaper_display_update(void) {
    if (framebuffer_bw == NULL || framebuffer_red == NULL) {
        ESP_LOGE("epaper", "Framebuffers not initialized");
        return;
    }

    ESP_LOGI("epaper", "Updating display from framebuffer...");

    // Debug: print first few bytes to verify data
    ESP_LOGI("epaper", "BW buffer first 8 bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
             framebuffer_bw[0], framebuffer_bw[1], framebuffer_bw[2], framebuffer_bw[3],
             framebuffer_bw[4], framebuffer_bw[5], framebuffer_bw[6], framebuffer_bw[7]);
    ESP_LOGI("epaper", "RED buffer first 8 bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
             framebuffer_red[0], framebuffer_red[1], framebuffer_red[2], framebuffer_red[3],
             framebuffer_red[4], framebuffer_red[5], framebuffer_red[6], framebuffer_red[7]);

    // Match epaper_fill() pattern which works
    // Fill BW channel (0x13) FIRST
    epaper_send_command(0x13);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        epaper_send_data(framebuffer_red[i]);
        // Yield every 1024 bytes to avoid watchdog
        if ((i % 1024) == 0 && i > 0) {
            vTaskDelay(1);
        }
    }

    // Fill RED channel (0x10) SECOND
    epaper_send_command(0x10);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        epaper_send_data(framebuffer_bw[i]);
        // Yield every 1024 bytes to avoid watchdog
        if ((i % 1024) == 0 && i > 0) {
            vTaskDelay(1);
        }
    }

    // Refresh display - use epaper_flushDisplay() pattern (includes power management)
    ESP_LOGI("epaper", "Refreshing display...");
    epaper_flushDisplay();
    ESP_LOGI("epaper", "Display update complete");
}

// Clear framebuffer to white
void epaper_display_clear(void) {
    epaper_framebuffer_init();
    memset(framebuffer_bw, 0x00, BUFFER_SIZE);
    memset(framebuffer_red, 0x00, BUFFER_SIZE);
    ESP_LOGI("epaper", "Framebuffer cleared");
}

// Set global screen orientation
void epaper_set_orientation(uint8_t orientation) {
    if (orientation <= ORIENTATION_270) {
        screen_orientation = orientation;
        ESP_LOGI("epaper", "Screen orientation set to %d°", orientation * 90);
    } else {
        ESP_LOGE("epaper", "Invalid orientation: %d", orientation);
    }
}

// Get current screen orientation
uint8_t epaper_get_orientation(void) {
    return screen_orientation;
}

// Test if partial updates work on this display
void epaper_test_partial_update(void) {
    ESP_LOGI("epaper", "=== Testing Partial Update Support ===");

    // Step 1: Full screen refresh with white
    ESP_LOGI("epaper", "1. Full refresh - white background");
    epaper_clearDisplay();
    vTaskDelay(pdMS_TO_TICKS(20000));

    // Step 2: Draw a black square at (0,0) with PARTIAL mode
    ESP_LOGI("epaper", "2. Partial update - black square at (0,0)");
    epaper_DCDC_powerOn();

    // Set partial window
    epaper_set_partial_window(0, 0, 64, 64);

    // Enter partial mode
    epaper_send_command(0x91);

    // Send just the partial window data (64x64 = 8 bytes wide * 64 rows = 512 bytes)
    uint32_t partial_size = (64 / 8) * 64;

    // BW channel - send red (0xFF)
    epaper_send_command(0x13);
    for (uint32_t i = 0; i < partial_size; i++) {
        epaper_send_data(0xFF);
    }

    // RED channel - send black (0x00)
    epaper_send_command(0x10);
    for (uint32_t i = 0; i < partial_size; i++) {
        epaper_send_data(0x00);
    }

    // Trigger partial refresh
    epaper_send_command(0x12);
    epaper_waitBusy();

    // Exit partial mode
    epaper_send_command(0x92);
    epaper_DCDC_powerOff();

    ESP_LOGI("epaper", "=== Test Results ===");
    ESP_LOGI("epaper", "If you see ONLY a black square at top-left:");
    ESP_LOGI("epaper", "  ✓ Partial updates WORK!");
    ESP_LOGI("epaper", "If the ENTIRE screen is black or has artifacts:");
    ESP_LOGI("epaper", "  ✗ Partial updates NOT supported (use full refresh only)");
    ESP_LOGI("epaper", "=====================");
}

// Text rendering functions
#include "font5x7.h"
#include "font6x12.h"
#include "font8x16.h"

// Helper function to transform coordinates based on orientation
static inline void transform_coordinates(uint16_t x, uint16_t y, uint8_t orientation, uint16_t *out_x, uint16_t *out_y) {
    switch (orientation) {
        case ORIENTATION_0:   // 0° - Normal
            *out_x = x;
            *out_y = y;
            break;
        case ORIENTATION_90:  // 90° clockwise
            *out_x = SCREEN_2_6_HEIGHT - 1 - y;
            *out_y = x;
            break;
        case ORIENTATION_180: // 180°
            *out_x = SCREEN_2_6_WIDTH - 1 - x;
            *out_y = SCREEN_2_6_HEIGHT - 1 - y;
            break;
        case ORIENTATION_270: // 270° clockwise (90° counter-clockwise)
            *out_x = y;
            *out_y = SCREEN_2_6_WIDTH - 1 - x;
            break;
        default:
            *out_x = x;
            *out_y = y;
            break;
    }
}

// Draw a single character to the framebuffer
// x, y: top-left corner of character
// c: character to draw
// color: COLOR_BLACK, COLOR_RED, or COLOR_WHITE
// scale: scaling factor (1 = normal, 2 = 2x, etc.)
void epaper_draw_char(uint16_t x, uint16_t y, char c, uint8_t color, uint8_t scale) {
    // Check if character is in font range
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) {
        c = '?'; // Replace unknown chars with question mark
    }

    // Get font data for this character
    const uint8_t *glyph = font5x7[c - FONT_FIRST_CHAR];

    // Draw each column of the character
    for (uint8_t col = 0; col < FONT_WIDTH; col++) {
        uint8_t column_data = glyph[col];

        // Draw each row (bit) in this column
        for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
            if (column_data & (1 << row)) {
                // Pixel is set - draw it (scaled)
                for (uint8_t sx = 0; sx < scale; sx++) {
                    for (uint8_t sy = 0; sy < scale; sy++) {
                        uint16_t px = x + (col * scale) + sx;
                        uint16_t py = y + (row * scale) + sy;

                        // Draw single pixel using epaper_rect (1x1 rectangle)
                        if (px < SCREEN_2_6_WIDTH && py < SCREEN_2_6_HEIGHT) {
                            epaper_rect(px, py, 1, 1, color);
                        }
                    }
                }
            }
        }
    }
}

// Draw a text string to the framebuffer (uses global orientation)
// x, y: top-left corner of first character
// text: null-terminated string
// color: COLOR_BLACK, COLOR_RED, or COLOR_WHITE
// scale: scaling factor (1 = normal, 2 = 2x, etc.)
void epaper_draw_text(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale) {
    if (text == NULL) return;

    uint8_t orientation = screen_orientation;
    uint16_t char_width = (FONT_WIDTH + 1) * scale;

    // For rotated text, calculate text length and adjust starting position
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    if (orientation != ORIENTATION_0) {
        const char *temp = text;
        uint16_t char_count = 0;
        while (*temp) {
            if ((unsigned char)*temp == 0xC2 || (unsigned char)*temp == 0xC3) {
                temp += 2;
                char_count++;
            } else if (*temp == '\n' || *temp == '\r') {
                temp++;
            } else {
                temp++;
                char_count++;
            }
        }
        switch (orientation) {
            case ORIENTATION_90:
                cursor_y = y + (char_count * char_width);
                break;
            case ORIENTATION_180:
                cursor_x = x + (char_count * char_width);
                break;
            case ORIENTATION_270:
                cursor_y = y + (char_count * char_width);
                break;
        }
    }

    // Calculate character advancement direction
    int16_t dx = 0, dy = 0;
    switch (orientation) {
        case ORIENTATION_0:   dx = char_width; dy = 0; break;
        case ORIENTATION_90:  dx = 0; dy = char_width; break;
        case ORIENTATION_180: dx = -char_width; dy = 0; break;
        case ORIENTATION_270: dx = 0; dy = -char_width; break;
    }

    while (*text) {
        uint8_t c;
        bool is_utf8 = false;

        if ((unsigned char)*text == 0xC2 && (unsigned char)*(text + 1) == 0xB0) {
            c = 127;
            is_utf8 = true;
        } else if ((unsigned char)*text == 0xC3 && (unsigned char)*(text + 1) == 0xA9) {
            c = 128;
            is_utf8 = true;
        } else if ((unsigned char)*text == 0xC3 && (unsigned char)*(text + 1) == 0xA8) {
            c = 129;
            is_utf8 = true;
        } else if (*text == '\n' || *text == '\r') {
            text++;
            continue;
        } else {
            c = *text;
        }

        if (c >= FONT_FIRST_CHAR && c <= FONT_LAST_CHAR) {
            const uint8_t *char_data = font5x7[c - FONT_FIRST_CHAR];

            for (uint8_t col = 0; col < FONT_WIDTH; col++) {
                uint8_t actual_col = (orientation == ORIENTATION_180) ? (FONT_WIDTH - 1 - col) : col;
                uint8_t column = char_data[actual_col];

                for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
                    uint8_t actual_row = (orientation == ORIENTATION_180) ? (FONT_HEIGHT - 1 - row) : row;
                    if (column & (1 << actual_row)) {
                        for (uint8_t sy = 0; sy < scale; sy++) {
                            for (uint8_t sx = 0; sx < scale; sx++) {
                                uint16_t px = cursor_x + col * scale + sx;
                                uint16_t py = cursor_y + row * scale + sy;
                                uint16_t out_x, out_y;
                                transform_coordinates(px, py, orientation, &out_x, &out_y);
                                epaper_draw_pixel_direct(out_x, out_y, color);
                            }
                        }
                    }
                }
            }
        }

        cursor_x += dx;
        cursor_y += dy;
        text += is_utf8 ? 2 : 1;
    }
}

// ========== 6x12 Font Rendering (medium size, good balance) ==========

// Draw a single character using 6x12 font to the framebuffer
// x, y: top-left corner of character
// c: character to draw
// color: COLOR_BLACK, COLOR_RED, or COLOR_WHITE
// scale: scaling factor (1 = normal, 2 = 2x, etc.)
void epaper_draw_char_6x12(uint16_t x, uint16_t y, char c, uint8_t color, uint8_t scale) {
    // Check if character is in font range
    if (c < FONT6X12_FIRST_CHAR || c > FONT6X12_LAST_CHAR) {
        c = '?'; // Replace unknown chars with question mark
    }

    // Get font data for this character
    const uint8_t *glyph = font6x12[c - FONT6X12_FIRST_CHAR];

    // Draw each row of the character
    for (uint8_t row = 0; row < FONT6X12_HEIGHT; row++) {
        uint8_t row_data = glyph[row];

        // Draw each column (bit) in this row
        for (uint8_t col = 0; col < FONT6X12_WIDTH; col++) {
            if (row_data & (0x80 >> col)) {
                // Pixel is set - draw it (scaled)
                for (uint8_t sx = 0; sx < scale; sx++) {
                    for (uint8_t sy = 0; sy < scale; sy++) {
                        uint16_t px = x + (col * scale) + sx;
                        uint16_t py = y + (row * scale) + sy;

                        // Draw single pixel using epaper_rect (1x1 rectangle)
                        if (px < SCREEN_2_6_WIDTH && py < SCREEN_2_6_HEIGHT) {
                            epaper_rect(px, py, 1, 1, color);
                        }
                    }
                }
            }
        }
    }
}

// Draw a text string using 6x12 font to the framebuffer (uses global orientation)
// x, y: top-left corner of first character
// text: null-terminated string
// color: COLOR_BLACK, COLOR_RED, or COLOR_WHITE
// scale: scaling factor (1 = normal, 2 = 2x, etc.)
void epaper_draw_text_6x12(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale) {
    if (text == NULL) return;

    uint8_t orientation = screen_orientation;
    uint16_t char_width = (FONT6X12_WIDTH + 1) * scale;

    // For rotated text, calculate text length and adjust starting position
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    if (orientation != ORIENTATION_0) {
        const char *temp = text;
        uint16_t char_count = 0;
        while (*temp) {
            if ((unsigned char)*temp == 0xC2 || (unsigned char)*temp == 0xC3) {
                temp += 2;
                char_count++;
            } else if (*temp == '\n' || *temp == '\r') {
                temp++;
            } else {
                temp++;
                char_count++;
            }
        }
        switch (orientation) {
            case ORIENTATION_90:
                cursor_y = y + (char_count * char_width);
                break;
            case ORIENTATION_180:
                cursor_x = x + (char_count * char_width);
                break;
            case ORIENTATION_270:
                cursor_y = y + (char_count * char_width);
                break;
        }
    }

    // Calculate character advancement direction
    int16_t dx = 0, dy = 0;
    switch (orientation) {
        case ORIENTATION_0:   dx = char_width; dy = 0; break;
        case ORIENTATION_90:  dx = 0; dy = char_width; break;
        case ORIENTATION_180: dx = -char_width; dy = 0; break;
        case ORIENTATION_270: dx = 0; dy = -char_width; break;
    }

    while (*text) {
        uint8_t c;
        bool is_utf8 = false;

        if ((unsigned char)*text == 0xC2 && (unsigned char)*(text + 1) == 0xB0) {
            c = 127;
            is_utf8 = true;
        } else if ((unsigned char)*text == 0xC3 && (unsigned char)*(text + 1) == 0xA9) {
            c = 128;
            is_utf8 = true;
        } else if ((unsigned char)*text == 0xC3 && (unsigned char)*(text + 1) == 0xA8) {
            c = 129;
            is_utf8 = true;
        } else if (*text == '\n' || *text == '\r') {
            text++;
            continue;
        } else {
            c = *text;
        }

        if (c >= FONT6X12_FIRST_CHAR && c <= FONT6X12_LAST_CHAR) {
            const uint8_t *char_data = font6x12[c - FONT6X12_FIRST_CHAR];

            for (uint8_t row = 0; row < FONT6X12_HEIGHT; row++) {
                uint8_t actual_row = (orientation == ORIENTATION_180) ? (FONT6X12_HEIGHT - 1 - row) : row;
                uint8_t line = char_data[actual_row];

                for (uint8_t col = 0; col < FONT6X12_WIDTH; col++) {
                    uint8_t actual_col = (orientation == ORIENTATION_180) ? (FONT6X12_WIDTH - 1 - col) : col;
                    if (line & (1 << actual_col)) {
                        for (uint8_t sy = 0; sy < scale; sy++) {
                            for (uint8_t sx = 0; sx < scale; sx++) {
                                uint16_t px = cursor_x + col * scale + sx;
                                uint16_t py = cursor_y + row * scale + sy;
                                uint16_t out_x, out_y;
                                transform_coordinates(px, py, orientation, &out_x, &out_y);
                                epaper_draw_pixel_direct(out_x, out_y, color);
                            }
                        }
                    }
                }
            }
        }

        cursor_x += dx;
        cursor_y += dy;
        text += is_utf8 ? 2 : 1;
    }
}

// ========== 8x16 Font Rendering (cleaner, more readable) ==========

// Draw a single character using 8x16 font to the framebuffer
// x, y: top-left corner of character
// c: character to draw
// color: COLOR_BLACK, COLOR_RED, or COLOR_WHITE
// scale: scaling factor (1 = normal, 2 = 2x, etc.)
void epaper_draw_char_8x16(uint16_t x, uint16_t y, char c, uint8_t color, uint8_t scale) {
    // Check if character is in font range
    if (c < FONT8X16_FIRST_CHAR || c > FONT8X16_LAST_CHAR) {
        c = '?'; // Replace unknown chars with question mark
    }

    // Get font data for this character
    const uint8_t *glyph = font8x16[c - FONT8X16_FIRST_CHAR];

    // Draw each row of the character
    for (uint8_t row = 0; row < FONT8X16_HEIGHT; row++) {
        uint8_t row_data = glyph[row];

        // Draw each column (bit) in this row
        for (uint8_t col = 0; col < FONT8X16_WIDTH; col++) {
            if (row_data & (0x80 >> col)) {
                // Pixel is set - draw it (scaled)
                for (uint8_t sx = 0; sx < scale; sx++) {
                    for (uint8_t sy = 0; sy < scale; sy++) {
                        uint16_t px = x + (col * scale) + sx;
                        uint16_t py = y + (row * scale) + sy;

                        // Draw single pixel using epaper_rect (1x1 rectangle)
                        if (px < SCREEN_2_6_WIDTH && py < SCREEN_2_6_HEIGHT) {
                            epaper_rect(px, py, 1, 1, color);
                        }
                    }
                }
            }
        }
    }
}

// Draw a text string using 8x16 font to the framebuffer (uses global orientation)
// x, y: top-left corner of first character
// text: null-terminated string
// color: COLOR_BLACK, COLOR_RED, or COLOR_WHITE
// scale: scaling factor (1 = normal, 2 = 2x, etc.)
void epaper_draw_text_8x16(uint16_t x, uint16_t y, const char *text, uint8_t color, uint8_t scale) {
    if (text == NULL) return;

    uint8_t orientation = screen_orientation;
    uint16_t char_width = (FONT8X16_WIDTH + 1) * scale;

    // For rotated text, calculate text length and adjust starting position
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    if (orientation != ORIENTATION_0) {
        const char *temp = text;
        uint16_t char_count = 0;
        while (*temp) {
            if ((unsigned char)*temp == 0xC2 || (unsigned char)*temp == 0xC3) {
                temp += 2;
                char_count++;
            } else if (*temp == '\n' || *temp == '\r') {
                temp++;
            } else {
                temp++;
                char_count++;
            }
        }
        switch (orientation) {
            case ORIENTATION_90:
                cursor_y = y + (char_count * char_width);
                break;
            case ORIENTATION_180:
                cursor_x = x + (char_count * char_width);
                break;
            case ORIENTATION_270:
                cursor_y = y + (char_count * char_width);
                break;
        }
    }

    // Calculate character advancement direction
    int16_t dx = 0, dy = 0;
    switch (orientation) {
        case ORIENTATION_0:   dx = char_width; dy = 0; break;
        case ORIENTATION_90:  dx = 0; dy = char_width; break;
        case ORIENTATION_180: dx = -char_width; dy = 0; break;
        case ORIENTATION_270: dx = 0; dy = -char_width; break;
    }

    while (*text) {
        uint8_t c;
        bool is_utf8 = false;

        if ((unsigned char)*text == 0xC2 && (unsigned char)*(text + 1) == 0xB0) {
            c = 127;
            is_utf8 = true;
        } else if ((unsigned char)*text == 0xC3 && (unsigned char)*(text + 1) == 0xA9) {
            c = 128;
            is_utf8 = true;
        } else if ((unsigned char)*text == 0xC3 && (unsigned char)*(text + 1) == 0xA8) {
            c = 129;
            is_utf8 = true;
        } else if (*text == '\n' || *text == '\r') {
            text++;
            continue;
        } else {
            c = *text;
        }

        if (c >= FONT8X16_FIRST_CHAR && c <= FONT8X16_LAST_CHAR) {
            const uint8_t *char_data = font8x16[c - FONT8X16_FIRST_CHAR];

            for (uint8_t row = 0; row < FONT8X16_HEIGHT; row++) {
                uint8_t actual_row = (orientation == ORIENTATION_180) ? (FONT8X16_HEIGHT - 1 - row) : row;
                uint8_t line = char_data[actual_row];

                for (uint8_t col = 0; col < FONT8X16_WIDTH; col++) {
                    uint8_t actual_col = (orientation == ORIENTATION_180) ? (FONT8X16_WIDTH - 1 - col) : col;
                    if (line & (1 << actual_col)) {
                        for (uint8_t sy = 0; sy < scale; sy++) {
                            for (uint8_t sx = 0; sx < scale; sx++) {
                                uint16_t px = cursor_x + col * scale + sx;
                                uint16_t py = cursor_y + row * scale + sy;
                                uint16_t out_x, out_y;
                                transform_coordinates(px, py, orientation, &out_x, &out_y);
                                epaper_draw_pixel_direct(out_x, out_y, color);
                            }
                        }
                    }
                }
            }
        }

        cursor_x += dx;
        cursor_y += dy;
        text += is_utf8 ? 2 : 1;
    }
}

