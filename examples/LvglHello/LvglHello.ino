#include <SSD1322_LVGL.h>
#include <lvgl.h>

constexpr int8_t PIN_CS = 5;
constexpr int8_t PIN_DC = 17;
constexpr int8_t PIN_RST = 16;

SSD1322_LVGL oled(SPI, PIN_CS, PIN_DC, PIN_RST, 16000000);

static void anim_cb(void *bar, int32_t value) {
  lv_bar_set_value(static_cast<lv_obj_t *>(bar), value, LV_ANIM_OFF);
}

void setup() {
  SPI.begin();
  oled.begin();

  lv_init();

  lv_display_t *disp = oled.createDisplay(16, LV_COLOR_FORMAT_L8);
  (void)disp;

  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello SSD1322");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 4);

  lv_obj_t *bar = lv_bar_create(lv_screen_active());
  lv_obj_set_size(bar, 200, 12);
  lv_obj_align(bar, LV_ALIGN_CENTER, 0, 10);
  lv_bar_set_range(bar, 0, 100);

  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, bar);
  lv_anim_set_exec_cb(&anim, anim_cb);
  lv_anim_set_values(&anim, 0, 100);
  lv_anim_set_time(&anim, 1500);
  lv_anim_set_playback_time(&anim, 1500);
  lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&anim);
}

void loop() {
  static uint32_t last_ms = 0;
  const uint32_t now = millis();
  const uint32_t diff = now - last_ms;
  last_ms = now;
  lv_tick_inc(diff);
  lv_timer_handler();
  delay(5);
}
