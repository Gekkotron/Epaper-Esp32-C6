#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace epaper_rwb {

// Tri-color (Red/White/Black) 2.6" e-paper display driver
// Resolution: 152x296, SPI interface
// Compatible with ESPHome display framework (fonts, shapes, lambda, pages)
//
// Color mapping in lambdas:
//   COLOR_ON  → Black pixel
//   COLOR_OFF → White pixel
//   Color(255, 0, 0) → Red pixel

class EpaperRWBDisplay : public display::DisplayBuffer,
                          public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                                spi::CLOCK_POLARITY_LOW,
                                                spi::CLOCK_PHASE_LEADING,
                                                spi::DATA_RATE_4MHZ> {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void fill(Color color) override;

  void set_dc_pin(GPIOPin *pin) { this->dc_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_busy_pin(GPIOPin *pin) { this->busy_pin_ = pin; }
  void set_power_pin(GPIOPin *pin) { this->power_pin_ = pin; }

  display::DisplayType get_display_type() override {
    return display::DisplayType::DISPLAY_TYPE_COLOR;
  }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override;
  int get_height_internal() override;
  uint32_t get_buffer_length_();

  void init_display_();
  void send_command_(uint8_t cmd);
  void send_data_(uint8_t data);
  void send_command_data_(uint8_t cmd, const uint8_t *data, uint32_t len);
  void wait_busy_();
  void hardware_reset_();
  void power_on_();
  void power_off_();

  GPIOPin *dc_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};
  GPIOPin *power_pin_{nullptr};

  // Physical display dimensions
  static const uint16_t WIDTH = 152;
  static const uint16_t HEIGHT = 296;
  static const uint16_t BYTES_PER_ROW = WIDTH / 8;          // 19
  static const uint32_t SINGLE_BUFFER_SIZE = BYTES_PER_ROW * HEIGHT;  // 5624
};

}  // namespace epaper_rwb
}  // namespace esphome
