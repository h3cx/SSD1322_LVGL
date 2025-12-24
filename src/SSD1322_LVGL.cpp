#include "SSD1322_LVGL.h"

namespace {
constexpr uint8_t kCmdDisplayOff = 0xAE;
constexpr uint8_t kCmdDisplayOn = 0xAF;
constexpr uint8_t kCmdSetColumnAddress = 0x15;
constexpr uint8_t kCmdSetRowAddress = 0x75;
constexpr uint8_t kCmdWriteRam = 0x5C;
constexpr uint8_t kCmdSetRemap = 0xA0;
constexpr uint8_t kCmdSetStartLine = 0xA1;
constexpr uint8_t kCmdSetDisplayOffset = 0xA2;
constexpr uint8_t kCmdSetDisplayClock = 0xB3;
constexpr uint8_t kCmdSetGpio = 0xB5;
constexpr uint8_t kCmdSetPhaseLength = 0xB1;
constexpr uint8_t kCmdSetPrechargePeriod = 0xB6;
constexpr uint8_t kCmdSetLinearGrayTable = 0xB9;
constexpr uint8_t kCmdSetPrechargeVoltage = 0xBB;
constexpr uint8_t kCmdSetVcomh = 0xBE;
constexpr uint8_t kCmdSetContrastCurrent = 0xC1;
constexpr uint8_t kCmdSetMasterCurrent = 0xC7;
constexpr uint8_t kCmdSetMultiplexRatio = 0xCA;
constexpr uint8_t kCmdSetDisplayEnhancementA = 0xB4;
constexpr uint8_t kCmdSetDisplayEnhancementB = 0xD1;
constexpr uint8_t kCmdSetFunctionSelection = 0xAB;
constexpr uint8_t kCmdSetCommandLock = 0xFD;
constexpr uint8_t kCmdSetNormalDisplay = 0xA6;
constexpr uint8_t kCmdExitPartialDisplay = 0xA9;

constexpr uint8_t kColumnOffset = 0x1C; // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
} // namespace

SSD1322_LVGL::SSD1322_LVGL(SPIClass &spi, int8_t cs, int8_t dc, int8_t rst, uint32_t hz)
    : spi_(spi), cs_(cs), dc_(dc), rst_(rst), hz_(hz) {}

bool SSD1322_LVGL::begin() {
  pinMode(cs_, OUTPUT);
  pinMode(dc_, OUTPUT);
  pinMode(rst_, OUTPUT);

  digitalWrite(cs_, HIGH);
  digitalWrite(dc_, HIGH);

  digitalWrite(rst_, LOW);
  delay(20);
  digitalWrite(rst_, HIGH);
  delay(50);

  writeCommand(kCmdDisplayOff);
  writeCommand(kCmdSetCommandLock, 0x12); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence

  // Column/row setup uses 0x1C..0x5B window for 256px width on SSD1322 480-column mapping.
  // (NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence)
  setWindow(0, 0, kWidth - 1, kHeight - 1);

  writeCommand(kCmdSetDisplayClock, 0x91); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetMultiplexRatio, 0x3F); // SSD1322.pdf p.47 "Set Multiplex Ratio"; panel sets 0x3F (NHD p.9)
  writeCommand(kCmdSetDisplayOffset, 0x00); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetStartLine, 0x00); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence

  // Remap format: 0x14 per panel example (horizontal address increment, dual COM line mode).
  // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetRemap, 0x14);

  writeCommand(kCmdSetGpio, 0x00); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetFunctionSelection, 0x01); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence

  const uint8_t enhance_a[] = {0xA0, 0xFD};
  writeCommand(kCmdSetDisplayEnhancementA, enhance_a, sizeof(enhance_a));
  writeCommand(kCmdSetContrastCurrent, 0x9F); // SSD1322.pdf p.47 "Set Contrast Current"; NHD p.9 example
  writeCommand(kCmdSetMasterCurrent, 0x0F); // SSD1322.pdf p.47 "Master Current Control"; NHD p.9 example
  writeCommand(kCmdSetLinearGrayTable); // SSD1322.pdf p.29 "Enable Linear Gray Scale Table"; NHD p.9 example
  writeCommand(kCmdSetPhaseLength, 0xE2); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetDisplayEnhancementB, 0x20); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetPrechargeVoltage, 0x1F); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetPrechargePeriod, 0x08); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetVcomh, 0x07); // NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence
  writeCommand(kCmdSetNormalDisplay); // SSD1322.pdf p.33 "Set Display Mode" (A6h = Normal Display)
  writeCommand(kCmdExitPartialDisplay); // SSD1322.pdf p.33 "Exit Partial Display"
  writeCommand(kCmdDisplayOn);

  return true;
}

void SSD1322_LVGL::setContrast(uint8_t c) {
  writeCommand(kCmdSetContrastCurrent, c); // SSD1322.pdf p.47 "Set Contrast Current"
}

void SSD1322_LVGL::fill(uint8_t gray4) {
  gray4 &= 0x0F;
  // Two pixels per byte: high nibble = left pixel, low nibble = right pixel.
  // SSD1322.pdf p.22 "GDDRAM structure in Gray Scale mode".
  const uint8_t packed = (gray4 << 4) | gray4;

  setWindow(0, 0, kWidth - 1, kHeight - 1);
  writeCommand(kCmdWriteRam); // SSD1322.pdf p.37 "Write RAM Command (5Ch)"

  uint8_t row[(kWidth / 2)] = {};
  memset(row, packed, sizeof(row));

  for (int16_t y = 0; y < kHeight; ++y) {
    writeData(row, sizeof(row));
  }
}

void SSD1322_LVGL::testPattern() {
  setWindow(0, 0, kWidth - 1, kHeight - 1);
  writeCommand(kCmdWriteRam); // SSD1322.pdf p.37 "Write RAM Command (5Ch)"

  uint8_t row[(kWidth / 2)] = {};

  for (int16_t y = 0; y < kHeight; ++y) {
    for (int16_t x = 0; x < kWidth; x += 2) {
      uint8_t g0 = static_cast<uint8_t>((x * 15) / (kWidth - 1));
      uint8_t g1 = static_cast<uint8_t>(((x + 1) * 15) / (kWidth - 1));

      if (y < 16) {
        g0 = 0;
        g1 = 0;
      } else if (y > 47) {
        g0 = 0x0F;
        g1 = 0x0F;
      }

      if (x < 32 && y >= 24 && y < 40) {
        g0 = 0x0F;
        g1 = 0x0F;
      }

      // Two pixels per byte: high nibble = left, low nibble = right.
      // SSD1322.pdf p.22 "GDDRAM structure in Gray Scale mode".
      row[x / 2] = static_cast<uint8_t>((g0 << 4) | g1);
    }
    writeData(row, sizeof(row));
  }
}

lv_display_t *SSD1322_LVGL::createDisplay(uint32_t buffer_lines, lv_color_format_t color_format) {
  if (buffer_lines == 0) {
    buffer_lines = 1;
  }
  if (buffer_lines > kMaxBufferLines) {
    buffer_lines = kMaxBufferLines;
  }

  const uint32_t bits_per_pixel = lv_color_format_get_size(color_format);
  const uint32_t buf_size = kWidth * buffer_lines * bits_per_pixel / 8;
  if (buf_size > sizeof(draw_buf_mem_)) {
    return nullptr;
  }

  disp_ = lv_display_create(kWidth, kHeight);
  lv_display_set_color_format(disp_, color_format);

  lv_draw_buf_init(&draw_buf_, kWidth, buffer_lines, color_format, 0, draw_buf_mem_, buf_size);
  lv_display_set_draw_buffers(disp_, &draw_buf_, nullptr);
  lv_display_set_user_data(disp_, this);
  lv_display_set_flush_cb(disp_, SSD1322_LVGL::flushCb);

  return disp_;
}

void SSD1322_LVGL::flushCb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  SSD1322_LVGL *self = static_cast<SSD1322_LVGL *>(lv_display_get_user_data(disp));
  if (!self) {
    lv_display_flush_ready(disp);
    return;
  }

  self->flushInternal(disp, area, px_map);
}

void SSD1322_LVGL::writeCommand(uint8_t cmd) {
  spi_.beginTransaction(SPISettings(hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);
  digitalWrite(dc_, LOW);
  spi_.transfer(cmd);
  digitalWrite(cs_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::writeCommand(uint8_t cmd, uint8_t data) {
  writeCommand(cmd, &data, 1);
}

void SSD1322_LVGL::writeCommand(uint8_t cmd, const uint8_t *data, size_t len) {
  spi_.beginTransaction(SPISettings(hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);
  digitalWrite(dc_, LOW);
  spi_.transfer(cmd);
  if (data && len) {
    digitalWrite(dc_, HIGH);
    for (size_t i = 0; i < len; ++i) {
      spi_.transfer(data[i]);
    }
  }
  digitalWrite(cs_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::writeData(const uint8_t *data, size_t len) {
  spi_.beginTransaction(SPISettings(hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);
  digitalWrite(dc_, HIGH);
  for (size_t i = 0; i < len; ++i) {
    spi_.transfer(data[i]);
  }
  digitalWrite(cs_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::setWindow(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= kWidth) x2 = kWidth - 1;
  if (y2 >= kHeight) y2 = kHeight - 1;

  // SSD1322 uses 4 pixels per column address (480 columns / 4 = 120 addresses).
  // colStart = 0x1C + (x / 4) for 256px panels (NHD-3.12-25664UCY2.pdf p.9 Example Initialization Sequence).
  // GDDRAM nibble mapping: two pixels per byte, four pixels per column address (SSD1322.pdf p.22 "GDDRAM structure").
  const uint8_t col_start = static_cast<uint8_t>(kColumnOffset + (x1 / 4));
  const uint8_t col_end = static_cast<uint8_t>(kColumnOffset + (x2 / 4));

  const uint8_t col_params[] = {col_start, col_end};
  writeCommand(kCmdSetColumnAddress, col_params, sizeof(col_params));
  // SSD1322.pdf p.37 "Set Column Address (15h)"

  const uint8_t row_params[] = {static_cast<uint8_t>(y1), static_cast<uint8_t>(y2)};
  writeCommand(kCmdSetRowAddress, row_params, sizeof(row_params));
  // SSD1322.pdf p.38 "Set Row Address (75h)"
}

void SSD1322_LVGL::flushInternal(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  int16_t x1 = area->x1;
  int16_t x2 = area->x2;
  int16_t y1 = area->y1;
  int16_t y2 = area->y2;

  if (x2 < 0 || y2 < 0 || x1 >= kWidth || y1 >= kHeight) {
    lv_display_flush_ready(disp);
    return;
  }

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= kWidth) x2 = kWidth - 1;
  if (y2 >= kHeight) y2 = kHeight - 1;

  // Expand to 4-pixel boundaries because SSD1322 column addresses are in 4-pixel units.
  const int16_t aligned_x1 = static_cast<int16_t>(x1 & ~0x3);
  const int16_t aligned_x2 = static_cast<int16_t>((x2 | 0x3));

  const int16_t orig_w = area->x2 - area->x1 + 1;
  const lv_color_format_t cf = lv_display_get_color_format(disp);

  auto gray4_at = [&](int16_t x, int16_t y) -> uint8_t {
    if (x < area->x1 || x > area->x2 || y < area->y1 || y > area->y2) {
      return 0;
    }
    const uint32_t offset = (y - area->y1) * orig_w + (x - area->x1);

    if (cf == LV_COLOR_FORMAT_L8) {
      return static_cast<uint8_t>(px_map[offset] >> 4);
    }

    if (cf == LV_COLOR_FORMAT_RGB565) {
      const uint16_t pixel = reinterpret_cast<const uint16_t *>(px_map)[offset];
      const uint8_t r = static_cast<uint8_t>((pixel >> 11) & 0x1F);
      const uint8_t g = static_cast<uint8_t>((pixel >> 5) & 0x3F);
      const uint8_t b = static_cast<uint8_t>(pixel & 0x1F);
      const uint8_t r8 = static_cast<uint8_t>((r << 3) | (r >> 2));
      const uint8_t g8 = static_cast<uint8_t>((g << 2) | (g >> 4));
      const uint8_t b8 = static_cast<uint8_t>((b << 3) | (b >> 2));
      const uint16_t gray8 = (static_cast<uint16_t>(r8) * 77 +
                              static_cast<uint16_t>(g8) * 150 +
                              static_cast<uint16_t>(b8) * 29) >> 8;
      return static_cast<uint8_t>(gray8 >> 4);
    }

    return 0;
  };

  setWindow(aligned_x1, y1, aligned_x2, y2);
  writeCommand(kCmdWriteRam); // SSD1322.pdf p.37 "Write RAM Command (5Ch)"

  const int16_t aligned_width = aligned_x2 - aligned_x1 + 1;
  const int16_t bytes_per_row = aligned_width / 2;
  uint8_t line_buf[kWidth / 2] = {};

  for (int16_t y = y1; y <= y2; ++y) {
    int16_t buf_index = 0;
    for (int16_t x = aligned_x1; x <= aligned_x2; x += 2) {
      const uint8_t g0 = gray4_at(x, y);
      const uint8_t g1 = gray4_at(x + 1, y);
      // Two pixels per byte: high nibble = left, low nibble = right.
      // SSD1322.pdf p.22 "GDDRAM structure in Gray Scale mode".
      line_buf[buf_index++] = static_cast<uint8_t>((g0 << 4) | g1);
    }
    writeData(line_buf, bytes_per_row);
  }

  lv_display_flush_ready(disp);
}
