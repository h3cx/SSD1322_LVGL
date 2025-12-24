# SSD1322_LVGL

Arduino library for SSD1322 256x64 grayscale OLEDs with LVGL 9.4 integration.
It provides:

- SPI driver with configurable CS/DC/RST pins.
- LVGL flush callback with 8bpp-to-4bpp conversion.
- Simple test helpers (full white, full black, test pattern).

## Hardware

- Controller: SSD1322
- Resolution: 256x64
- Pixel format: 4 bits per pixel (grayscale)
- Interface: SPI

## Installation

1. Copy this library folder into your Arduino `libraries` directory.
2. Install LVGL for Arduino (Library Manager -> "lvgl").
3. Ensure your `lv_conf.h` sets `LV_COLOR_DEPTH` to 8 and uses
   `LV_COLOR_FORMAT_L8` in your display setup.

## Quick start

```cpp
#include <lvgl.h>
#include <SSD1322_LVGL.h>

constexpr int8_t PIN_CS = 5;
constexpr int8_t PIN_DC = 17;
constexpr int8_t PIN_RST = 16;

SSD1322_LVGL display(SPI, PIN_CS, PIN_DC, PIN_RST, 8000000);

static uint8_t lvgl_buf[SSD1322_LVGL::kWidth * 32];

void setup() {
  lv_init();
  display.begin();

  lv_display_t *disp = lv_display_create(SSD1322_LVGL::kWidth,
                                         SSD1322_LVGL::kHeight);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_L8);
  lv_display_set_buffers(disp, lvgl_buf, nullptr, sizeof(lvgl_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_user_data(disp, &display);
  lv_display_set_flush_cb(disp, SSD1322_LVGL::lvglFlush);
  lv_display_add_event_cb(disp, SSD1322_LVGL::lvglInvalidateArea,
                          LV_EVENT_INVALIDATE_AREA, nullptr);
}

void loop() {
  lv_timer_handler();
  delay(5);
}
```

## LVGL buffer and color format

LVGL renders into an 8-bit luminance buffer (`LV_COLOR_FORMAT_L8`).
The driver expects this format and converts each 8-bit pixel to a 4-bit grayscale
value before sending it over SPI. Other LVGL color formats are not supported.

The invalidate-area callback ensures the LVGL render area always aligns to
2-pixel boundaries so that 4bpp packed bytes are correctly formed for the
SSD1322. This uses LVGL 9.4's `lv_display_add_event_cb` with
`LV_EVENT_INVALIDATE_AREA`.

## Test helpers

- `fillBlack()` / `fillWhite()` - Fill the panel with solid black or white.
- `fill(gray4)` - Fill the panel with a 4-bit grayscale (0x0-0xF).
- `testPattern()` - Render a simple bar pattern to verify wiring and contrast.

## Wiring notes

Typical SSD1322 SPI wiring (verify your module):

- `SCK` -> ESP32 SCK
- `MOSI` -> ESP32 MOSI
- `CS` -> user-defined (constructor)
- `DC` -> user-defined (constructor)
- `RST` -> user-defined (constructor)
- `GND` / `VCC` -> ground and 3.3V

## API

```cpp
SSD1322_LVGL(SPIClass &spi,
            int8_t cs_pin,
            int8_t dc_pin,
            int8_t rst_pin,
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
```

## Notes

- This library assumes the SSD1322 is in 4bpp mode and does not use any internal
  frame buffer; LVGL provides the render buffer.
- Adjust `spi_freq_hz` if your wiring or module needs a slower bus.

## License

MIT (add your preferred license file as needed).
