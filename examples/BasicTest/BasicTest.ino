#include <SSD1322_LVGL.h>

constexpr int8_t PIN_CS = 5;
constexpr int8_t PIN_DC = 17;
constexpr int8_t PIN_RST = 16;

SSD1322_LVGL oled(SPI, PIN_CS, PIN_DC, PIN_RST, 16000000);

void setup() {
  SPI.begin();
  oled.begin();

  oled.fill(0x0);
  delay(500);
  oled.fill(0xF);
  delay(500);
  oled.testPattern();
}

void loop() {
  // Nothing to do in this basic hardware test.
}
