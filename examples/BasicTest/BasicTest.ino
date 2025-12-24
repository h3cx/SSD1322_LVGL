#include <lvgl.h>
#include <SSD1322_LVGL.h>

// Configure your ESP32 pins here.
constexpr int8_t PIN_CS = 5;
constexpr int8_t PIN_DC = 17;
constexpr int8_t PIN_RST = 16;

SSD1322_LVGL display(SPI, PIN_CS, PIN_DC, PIN_RST, 8000000);

static uint8_t lvgl_buf[SSD1322_LVGL::kWidth * SSD1322_LVGL::kHeight];
static lv_display_t *lvgl_display = nullptr;

void setup() {
  Serial.begin(115200);
  lv_init();

  display.begin();
  display.fillBlack();
  delay(500);
  display.fillWhite();
  delay(500);
  display.testPattern();
  delay(1000);
  display.fillBlack();

  lvgl_display = lv_display_create(SSD1322_LVGL::kWidth,
                                   SSD1322_LVGL::kHeight);
  lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_L8);
  lv_display_set_buffers(lvgl_display, lvgl_buf, nullptr, sizeof(lvgl_buf),
                         LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_user_data(lvgl_display, &display);
  lv_display_set_flush_cb(lvgl_display, SSD1322_LVGL::lvglFlush);

  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "SSD1322 LVGL OK");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void loop() {
  lv_timer_handler();
  delay(5);
}
