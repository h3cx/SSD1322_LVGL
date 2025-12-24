#include "SSD1322_LVGL.h"

namespace {
constexpr uint8_t kCmdDisplayOff = 0xAE;
constexpr uint8_t kCmdDisplayOn = 0xAF;
constexpr uint8_t kCmdSetColumnAddress = 0x15;
constexpr uint8_t kCmdSetRowAddress = 0x75;
constexpr uint8_t kCmdSetContrast = 0xC1;
constexpr uint8_t kCmdSetRemap = 0xA0;
constexpr uint8_t kCmdSetDisplayStartLine = 0xA1;
constexpr uint8_t kCmdSetDisplayOffset = 0xA2;
constexpr uint8_t kCmdSetDisplayModeNormal = 0xA6;
constexpr uint8_t kCmdSetMultiplexRatio = 0xCA;
constexpr uint8_t kCmdSetMasterCurrent = 0xC7;
constexpr uint8_t kCmdSetClockDivide = 0xB3;
constexpr uint8_t kCmdSetPrecharge = 0xB1;
constexpr uint8_t kCmdSetVcomh = 0xBE;
constexpr uint8_t kCmdEnterNormalMode = 0xA4;
constexpr uint8_t kCmdSetGPIO = 0xB5;
constexpr uint8_t kCmdSetVsl = 0xB4;
constexpr uint8_t kCmdSetCommandLock = 0xFD;
constexpr uint8_t kCmdSetPrechargeVoltage = 0xBB;
constexpr uint8_t kCmdSetNormalDisplay = 0xA6;
constexpr uint8_t kCmdSetInverseDisplay = 0xA7;
}  // namespace

SSD1322_LVGL::SSD1322_LVGL(SPIClass &spi, int8_t cs_pin, int8_t dc_pin,
                           int8_t rst_pin, uint32_t spi_freq_hz)
    : spi_(spi),
      cs_pin_(cs_pin),
      dc_pin_(dc_pin),
      rst_pin_(rst_pin),
      spi_freq_hz_(spi_freq_hz) {}

bool SSD1322_LVGL::begin() {
  pinMode(cs_pin_, OUTPUT);
  pinMode(dc_pin_, OUTPUT);
  if (rst_pin_ >= 0) {
    pinMode(rst_pin_, OUTPUT);
  }

  digitalWrite(cs_pin_, HIGH);
  digitalWrite(dc_pin_, HIGH);

  spi_.begin();
  reset();
  initDisplay();
  return true;
}

void SSD1322_LVGL::reset() {
  if (rst_pin_ < 0) {
    return;
  }
  digitalWrite(rst_pin_, HIGH);
  delay(1);
  digitalWrite(rst_pin_, LOW);
  delay(10);
  digitalWrite(rst_pin_, HIGH);
  delay(10);
}

void SSD1322_LVGL::setContrast(uint8_t level) {
  uint8_t data = level;
  writeCommand(kCmdSetContrast, &data, 1);
}

void SSD1322_LVGL::setInverted(bool inverted) {
  writeCommand(inverted ? kCmdSetInverseDisplay : kCmdSetNormalDisplay);
}

void SSD1322_LVGL::fill(uint8_t gray4) {
  gray4 &= 0x0F;
  uint8_t packed = static_cast<uint8_t>((gray4 << 4) | gray4);
  setWindow(0, kWidth - 1, 0, kHeight - 1);

  const size_t bytes_per_row = kWidth / 2;
  const size_t total_bytes = bytes_per_row * kHeight;

  spi_.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_pin_, LOW);
  digitalWrite(dc_pin_, HIGH);
  for (size_t i = 0; i < total_bytes; ++i) {
    spi_.transfer(packed);
  }
  digitalWrite(cs_pin_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::fillBlack() {
  fill(0x00);
}

void SSD1322_LVGL::fillWhite() {
  fill(0x0F);
}

void SSD1322_LVGL::testPattern() {
  setWindow(0, kWidth - 1, 0, kHeight - 1);

  spi_.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_pin_, LOW);
  digitalWrite(dc_pin_, HIGH);

  for (uint16_t y = 0; y < kHeight; ++y) {
    for (uint16_t x = 0; x < kWidth; x += 2) {
      uint8_t left = (x / 16) & 0x0F;
      uint8_t right = ((x + 1) / 16) & 0x0F;
      spi_.transfer(static_cast<uint8_t>((left << 4) | right));
    }
  }

  digitalWrite(cs_pin_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::lvglFlush(lv_display_t *disp, const lv_area_t *area,
                            uint8_t *color_p) {
  auto *panel = static_cast<SSD1322_LVGL *>(lv_display_get_user_data(disp));
  if (!panel) {
    lv_display_flush_ready(disp);
    return;
  }

  const uint16_t x1 = static_cast<uint16_t>(area->x1);
  const uint16_t x2 = static_cast<uint16_t>(area->x2);
  const uint16_t y1 = static_cast<uint16_t>(area->y1);
  const uint16_t y2 = static_cast<uint16_t>(area->y2);

  panel->setWindow(x1, x2, y1, y2);

  const uint16_t width = static_cast<uint16_t>(x2 - x1 + 1);
  const uint16_t height = static_cast<uint16_t>(y2 - y1 + 1);

  panel->spi_.beginTransaction(
      SPISettings(panel->spi_freq_hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(panel->cs_pin_, LOW);
  digitalWrite(panel->dc_pin_, HIGH);

  size_t index = 0;
  for (uint16_t y = 0; y < height; ++y) {
    uint8_t packed = 0;
    bool high_nibble = true;
    for (uint16_t x = 0; x < width; ++x) {
      uint8_t l8 = color_p[index++];
      uint8_t l4 = static_cast<uint8_t>(l8 >> 4);
      if (high_nibble) {
        packed = static_cast<uint8_t>(l4 << 4);
        high_nibble = false;
      } else {
        packed = static_cast<uint8_t>(packed | l4);
        panel->spi_.transfer(packed);
        high_nibble = true;
      }
    }
    if (!high_nibble) {
      panel->spi_.transfer(packed);
    }
  }

  digitalWrite(panel->cs_pin_, HIGH);
  panel->spi_.endTransaction();

  lv_display_flush_ready(disp);
}

void SSD1322_LVGL::lvglInvalidateArea(lv_event_t *event) {
  lv_area_t *area = lv_event_get_invalidated_area(event);
  if (!area) {
    return;
  }
  area->x1 &= ~1;
  area->x2 |= 1;
  if (area->x2 >= static_cast<int32_t>(kWidth)) {
    area->x2 = static_cast<int32_t>(kWidth - 1);
  }
  if (area->x1 < 0) {
    area->x1 = 0;
  }
}

void SSD1322_LVGL::writeCommand(uint8_t command) {
  spi_.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_pin_, LOW);
  digitalWrite(dc_pin_, LOW);
  spi_.transfer(command);
  digitalWrite(cs_pin_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::writeCommand(uint8_t command, const uint8_t *data,
                                size_t len) {
  spi_.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_pin_, LOW);
  digitalWrite(dc_pin_, LOW);
  spi_.transfer(command);
  digitalWrite(dc_pin_, HIGH);
  for (size_t i = 0; i < len; ++i) {
    spi_.transfer(data[i]);
  }
  digitalWrite(cs_pin_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::writeData(const uint8_t *data, size_t len) {
  spi_.beginTransaction(SPISettings(spi_freq_hz_, MSBFIRST, SPI_MODE0));
  digitalWrite(cs_pin_, LOW);
  digitalWrite(dc_pin_, HIGH);
  for (size_t i = 0; i < len; ++i) {
    spi_.transfer(data[i]);
  }
  digitalWrite(cs_pin_, HIGH);
  spi_.endTransaction();
}

void SSD1322_LVGL::setWindow(uint16_t x1, uint16_t x2, uint16_t y1,
                             uint16_t y2) {
  uint8_t col_start = static_cast<uint8_t>(x1 / 2);
  uint8_t col_end = static_cast<uint8_t>(x2 / 2);
  uint8_t row_start = static_cast<uint8_t>(y1);
  uint8_t row_end = static_cast<uint8_t>(y2);

  uint8_t col_data[2] = {col_start, col_end};
  uint8_t row_data[2] = {row_start, row_end};

  writeCommand(kCmdSetColumnAddress, col_data, sizeof(col_data));
  writeCommand(kCmdSetRowAddress, row_data, sizeof(row_data));
}

void SSD1322_LVGL::initDisplay() {
  writeCommand(kCmdDisplayOff);

  uint8_t command_lock = 0x12;  // Unlock OLED driver.
  writeCommand(kCmdSetCommandLock, &command_lock, 1);

  uint8_t clock_div = 0x91;
  writeCommand(kCmdSetClockDivide, &clock_div, 1);

  uint8_t multiplex = kHeight - 1;
  writeCommand(kCmdSetMultiplexRatio, &multiplex, 1);

  uint8_t offset = 0x00;
  writeCommand(kCmdSetDisplayOffset, &offset, 1);

  uint8_t start_line = 0x00;
  writeCommand(kCmdSetDisplayStartLine, &start_line, 1);

  uint8_t remap[2] = {0x14, 0x11};
  writeCommand(kCmdSetRemap, remap, sizeof(remap));

  uint8_t gpio = 0x00;
  writeCommand(kCmdSetGPIO, &gpio, 1);

  uint8_t contrast = 0x7F;
  writeCommand(kCmdSetContrast, &contrast, 1);

  uint8_t master_current = 0x0F;
  writeCommand(kCmdSetMasterCurrent, &master_current, 1);

  uint8_t precharge = 0xE2;
  writeCommand(kCmdSetPrecharge, &precharge, 1);

  uint8_t vcomh = 0x0F;
  writeCommand(kCmdSetVcomh, &vcomh, 1);

  uint8_t precharge_volt = 0x1F;
  writeCommand(kCmdSetPrechargeVoltage, &precharge_volt, 1);

  uint8_t vsl[3] = {0xA0, 0xB5, 0x55};
  writeCommand(kCmdSetVsl, vsl, sizeof(vsl));

  writeCommand(kCmdEnterNormalMode);
  writeCommand(kCmdSetDisplayModeNormal);
  writeCommand(kCmdDisplayOn);
}
