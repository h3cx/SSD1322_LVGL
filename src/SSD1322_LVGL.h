#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>

class SSD1322_LVGL {
public:
  static constexpr int16_t kWidth = 256;
  static constexpr int16_t kHeight = 64;

  SSD1322_LVGL(SPIClass &spi, int8_t cs, int8_t dc, int8_t rst, uint32_t hz = 16000000);

  bool begin();
  void setContrast(uint8_t c);
  void fill(uint8_t gray4);   // 0..15
  void testPattern();

  // LVGL v9.4 integration
  lv_display_t *createDisplay(uint32_t buffer_lines = 16,
                              lv_color_format_t color_format = LV_COLOR_FORMAT_L8);

  static void flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

private:
  void writeCommand(uint8_t cmd);
  void writeCommand(uint8_t cmd, uint8_t data);
  void writeCommand(uint8_t cmd, const uint8_t *data, size_t len);
  void writeData(const uint8_t *data, size_t len);
  void setWindow(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
  void flushInternal(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

  SPIClass &spi_;
  int8_t cs_;
  int8_t dc_;
  int8_t rst_;
  uint32_t hz_;

  lv_display_t *disp_ = nullptr;
  lv_draw_buf_t draw_buf_{};

  static constexpr uint32_t kMaxBufferLines = 16;
  static constexpr uint32_t kMaxBytesPerPixel = 2; // RGB565 worst-case
  uint8_t draw_buf_mem_[kWidth * kMaxBufferLines * kMaxBytesPerPixel]{};
};
