#include "epaper_rwb.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include <cstring>

namespace esphome {
namespace epaper_rwb {

static const char *const TAG = "epaper_rwb";

// Hardware register data from original driver
// [0]=0x00  [1]=0x0e (soft reset)  [2]=0x19 (temp 25°C)
// [3]=0x02 (active temp)  [4]=0xcf [5]=0x8d (PSR)
static const uint8_t REGISTER_DATA[] = {0x00, 0x0e, 0x19, 0x02, 0xcf, 0x8d};

// ============================================================
// Setup & initialization
// ============================================================

void EpaperRWBDisplay::setup() {
  ESP_LOGD(TAG, "Setting up e-paper RWB display (152x296)...");

  // Allocate dual framebuffer (BW + RED)
  this->init_internal_(this->get_buffer_length_());

  // Init SPI bus
  this->spi_setup();

  // Init GPIO pins
  if (this->dc_pin_ != nullptr) {
    this->dc_pin_->setup();
    this->dc_pin_->digital_write(false);
  }
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
  }
  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();
  }
  if (this->power_pin_ != nullptr) {
    this->power_pin_->setup();
  }

  // Run display init sequence
  this->init_display_();

  ESP_LOGD(TAG, "E-paper RWB display ready");
}

void EpaperRWBDisplay::init_display_() {
  // Enable power (configure pin as active-low with inverted: true in YAML)
  if (this->power_pin_ != nullptr) {
    this->power_pin_->digital_write(true);
    ESP_LOGD(TAG, "Display power enabled");
  }

  delay(5);

  // Hardware reset
  this->hardware_reset_();

  // Wait for display to be ready
  this->wait_busy_();

  // Soft reset: command 0x00, data 0x0e
  uint8_t soft_reset_data = REGISTER_DATA[1];
  this->send_command_data_(0x00, &soft_reset_data, 1);
  this->wait_busy_();

  // Input temperature: 25°C
  uint8_t temp_data = REGISTER_DATA[2];
  this->send_command_data_(0xe5, &temp_data, 1);

  // Active temperature
  uint8_t active_temp = REGISTER_DATA[3];
  this->send_command_data_(0xe0, &active_temp, 1);

  // Panel Setting Register (PSR)
  uint8_t psr_data[2] = {REGISTER_DATA[4], REGISTER_DATA[5]};
  this->send_command_data_(0x00, psr_data, 2);

  ESP_LOGD(TAG, "Display hardware initialized");
}

void EpaperRWBDisplay::hardware_reset_() {
  if (this->reset_pin_ == nullptr) {
    return;
  }
  delay(1);
  this->reset_pin_->digital_write(false);
  delay(5);
  this->reset_pin_->digital_write(true);
  delay(10);
  this->reset_pin_->digital_write(false);
  delay(5);
  this->reset_pin_->digital_write(true);
  delay(1);
}

float EpaperRWBDisplay::get_setup_priority() const {
  return setup_priority::PROCESSOR;
}

// ============================================================
// SPI communication
// ============================================================

void EpaperRWBDisplay::send_command_(uint8_t cmd) {
  this->dc_pin_->digital_write(false);  // Command mode
  this->enable();
  this->write_byte(cmd);
  this->disable();
}

void EpaperRWBDisplay::send_data_(uint8_t data) {
  this->dc_pin_->digital_write(true);  // Data mode
  this->enable();
  this->write_byte(data);
  this->disable();
}

void EpaperRWBDisplay::send_command_data_(uint8_t cmd, const uint8_t *data, uint32_t len) {
  // Match original epaper_sendIndexData: command, then CS stays managed separately for data
  this->dc_pin_->digital_write(false);  // Command mode
  this->enable();
  this->write_byte(cmd);
  this->disable();

  this->dc_pin_->digital_write(true);  // Data mode
  this->enable();
  if (len > 0 && data != nullptr) {
    this->write_array(data, len);
  }
  this->disable();
}

void EpaperRWBDisplay::wait_busy_() {
  if (this->busy_pin_ == nullptr) {
    delay(200);
    return;
  }
  uint32_t start = millis();
  // BUSY pin HIGH = busy, LOW = ready
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > 10000) {
      ESP_LOGW(TAG, "Busy pin timeout (>10s)!");
      break;
    }
    delay(2);
    App.feed_wdt();
  }
  delay(200);
}

void EpaperRWBDisplay::power_on_() {
  uint8_t data = REGISTER_DATA[0];  // 0x00
  this->send_command_data_(0x04, &data, 1);
  this->wait_busy_();
}

void EpaperRWBDisplay::power_off_() {
  this->send_command_(0x02);
  this->wait_busy_();
}

// ============================================================
// Display update
// ============================================================

void EpaperRWBDisplay::update() {

  // do_update_() clears the buffer (if auto_clear) then calls the user lambda
  this->do_update_();

  // === DEBUG: Check buffer contents after lambda ===
  uint32_t bw_nonzero = 0, red_nonzero = 0;
  for (uint32_t i = 0; i < SINGLE_BUFFER_SIZE; i++) {
    if (this->buffer_[i] != 0x00)
      bw_nonzero++;
    if (this->buffer_[SINGLE_BUFFER_SIZE + i] != 0x00)
      red_nonzero++;
  }
  ESP_LOGW(TAG, "Buffer after lambda: BW non-zero=%u, RED non-zero=%u (of %u)", bw_nonzero, red_nonzero, SINGLE_BUFFER_SIZE);

  ESP_LOGD(TAG, "Sending framebuffer to display...");

  // Send RED buffer via command 0x13
  // (buffer layout: [0..SINGLE_BUFFER_SIZE-1]=BW, [SINGLE_BUFFER_SIZE..2*SINGLE_BUFFER_SIZE-1]=RED)
  this->send_command_(0x13);
  this->dc_pin_->digital_write(true);
  this->enable();
  for (uint32_t offset = 0; offset < SINGLE_BUFFER_SIZE; offset += 256) {
    uint32_t len = SINGLE_BUFFER_SIZE - offset;
    if (len > 256)
      len = 256;
    this->write_array(this->buffer_ + SINGLE_BUFFER_SIZE + offset, len);
    App.feed_wdt();
  }
  this->disable();

  // Send BW buffer via command 0x10
  this->send_command_(0x10);
  this->dc_pin_->digital_write(true);
  this->enable();
  for (uint32_t offset = 0; offset < SINGLE_BUFFER_SIZE; offset += 256) {
    uint32_t len = SINGLE_BUFFER_SIZE - offset;
    if (len > 256)
      len = 256;
    this->write_array(this->buffer_ + offset, len);
    App.feed_wdt();
  }
  this->disable();

  // Refresh display
  ESP_LOGD(TAG, "Power on + refresh...");
  this->power_on_();
  this->send_command_(0x12);
  ESP_LOGD(TAG, "Waiting for busy after refresh...");
  this->wait_busy_();
  this->power_off_();

  ESP_LOGD(TAG, "Display update complete");
}

// ============================================================
// Pixel drawing
// ============================================================

void EpaperRWBDisplay::fill(Color color) {
  if (this->buffer_ == nullptr)
    return;

  ESP_LOGD(TAG, "fill() called with r=%u g=%u b=%u", color.r, color.g, color.b);

  bool is_red = (color.r > 127 && color.g < 64 && color.b < 64);
  bool is_on = (color.r > 0 || color.g > 0 || color.b > 0);

  uint8_t bw_val = (is_on && !is_red) ? 0xFF : 0x00;
  uint8_t red_val = is_red ? 0xFF : 0x00;

  memset(this->buffer_, bw_val, SINGLE_BUFFER_SIZE);
  memset(this->buffer_ + SINGLE_BUFFER_SIZE, red_val, SINGLE_BUFFER_SIZE);
}

void EpaperRWBDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= this->get_width_internal() ||
      y < 0 || y >= this->get_height_internal()) {
    return;
  }
  if (this->buffer_ == nullptr)
    return;

  // DEBUG: log first few pixel draws
  static uint32_t pixel_count = 0;
  pixel_count++;
  if (pixel_count <= 5 || (pixel_count % 1000) == 0) {
    ESP_LOGD(TAG, "draw_pixel(%d, %d, r=%u g=%u b=%u) count=%u",
             x, y, color.r, color.g, color.b, pixel_count);
  }

  uint32_t byte_idx = y * BYTES_PER_ROW + (x / 8);
  uint8_t bit_mask = 0x80 >> (x % 8);

  // Color mapping:
  //   Red (r>127, g<64, b<64)         → RED pixel  (BW=0, RED=1)
  //   Any other non-zero / COLOR_ON   → BLACK pixel (BW=1, RED=0)
  //   Zero / COLOR_OFF                → WHITE pixel (BW=0, RED=0)
  bool is_red = (color.r > 127 && color.g < 64 && color.b < 64);
  bool is_on = (color.r > 0 || color.g > 0 || color.b > 0);

  if (is_red) {
    this->buffer_[byte_idx] &= ~bit_mask;
    this->buffer_[SINGLE_BUFFER_SIZE + byte_idx] |= bit_mask;
  } else if (is_on) {
    this->buffer_[byte_idx] |= bit_mask;
    this->buffer_[SINGLE_BUFFER_SIZE + byte_idx] &= ~bit_mask;
  } else {
    this->buffer_[byte_idx] &= ~bit_mask;
    this->buffer_[SINGLE_BUFFER_SIZE + byte_idx] &= ~bit_mask;
  }
}

// ============================================================
// Geometry
// ============================================================

int EpaperRWBDisplay::get_width_internal() { return WIDTH; }
int EpaperRWBDisplay::get_height_internal() { return HEIGHT; }
uint32_t EpaperRWBDisplay::get_buffer_length_() { return 2 * SINGLE_BUFFER_SIZE; }

// ============================================================
// Diagnostics
// ============================================================

void EpaperRWBDisplay::dump_config() {
  LOG_DISPLAY("", "E-Paper RWB 2.6\" (152x296)", this);
  ESP_LOGCONFIG(TAG, "  Resolution: %dx%d", WIDTH, HEIGHT);
  ESP_LOGCONFIG(TAG, "  Buffer size: %u bytes (BW + RED)", 2 * SINGLE_BUFFER_SIZE);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_PIN("  Power Pin: ", this->power_pin_);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace epaper_rwb
}  // namespace esphome
