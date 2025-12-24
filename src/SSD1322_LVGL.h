#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>

class SSD1322_LVGL {
 public:
  static constexpr uint16_t kWidth = 256;
  static constexpr uint16_t kHeight = 64;

  SSD1322_LVGL(SPIClass &spi, int8_t cs_pin, int8_t dc_pin, int8_t rst_pin,
              uint32_t spi_freq_hz = 8000000);

  bool begin();
  void reset();
  void setContrast(uint8_t level);
  void setInverted(bool inverted);

  void fill(uint8_t gray4);
  void fillBlack();
  void fillWhite();
  void testPattern();

  static void lvglFlush(lv_display_t *disp, const lv_area_t *area,
                        uint8_t *color_p);
  static void lvglRounder(lv_display_t *disp, lv_area_t *area);

 private:
  SPIClass &spi_;
  int8_t cs_pin_;
  int8_t dc_pin_;
  int8_t rst_pin_;
  uint32_t spi_freq_hz_;

  void writeCommand(uint8_t command);
  void writeCommand(uint8_t command, const uint8_t *data, size_t len);
  void writeData(const uint8_t *data, size_t len);
  void setWindow(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);
  void initDisplay();

  static uint8_t lvglColorToL8(lv_color_t color);
};
