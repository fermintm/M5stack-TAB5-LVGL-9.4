#include <Arduino.h>
#include <lvgl.h>
#include <M5Unified.h>

static lv_display_t *disp;
static lv_color_t *buf1;
static lv_color_t *buf2;
static lv_indev_t *indev;
static lv_obj_t *brightness_label;

static int32_t last_x = 0;
static int32_t last_y = 0;
static bool is_pressed = false;

static uint32_t my_tick_get_cb(void) {
  return millis();
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  M5.Display.startWrite();
  M5.Display.setAddrWindow(area->x1, area->y1, w, h);
  M5.Display.pushPixels((uint16_t *)px_map, w * h, true);
  M5.Display.endWrite();
  lv_display_flush_ready(disp);
}

void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
  data->point.x = last_x;
  data->point.y = last_y;
  data->state = is_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void brightness_slider_event_cb(lv_event_t *e) {
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  int32_t value = lv_slider_get_value(slider);
  M5.Display.setBrightness(value);
  lv_label_set_text_fmt(brightness_label, "Brillo: %d", value);
}

void ui_init(void) {
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "HELLO WORLD");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

  lv_obj_t *slider = lv_slider_create(scr);
  lv_obj_set_size(slider, 400, 50);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
  lv_slider_set_range(slider, 10, 255);
  lv_slider_set_value(slider, 200, LV_ANIM_OFF);

  lv_obj_set_style_bg_color(slider, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(0x00BFFF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 20, LV_PART_KNOB);

  lv_obj_add_event_cb(slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  brightness_label = lv_label_create(scr);
  lv_label_set_text_fmt(brightness_label, "BRIGHT: %d", 200);
  lv_obj_set_style_text_color(brightness_label, lv_color_hex(0x00FF00), 0);
  lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_24, 0);
  lv_obj_align(brightness_label, LV_ALIGN_CENTER, 0, 80);
}

void setup() {
  Serial.begin(115200);

  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(3);
  delay(100);

  uint16_t screenWidth = M5.Display.width();
  uint16_t screenHeight = M5.Display.height();

  Serial.printf("Pantalla: %d x %d\n", screenWidth, screenHeight);

  lv_init();
  lv_tick_set_cb(my_tick_get_cb);

  // ===== BUFFERS MÁS GRANDES CON PSRAM =====
  size_t buffer_pixels = screenWidth * 80;

  buf1 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  buf2 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

  if (!buf1 || !buf2) {
    Serial.println("PSRAM no disponible, usando DMA");
    if (buf1) heap_caps_free(buf1);
    if (buf2) heap_caps_free(buf2);

    buffer_pixels = screenWidth * 40;
    buf1 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
  }

  Serial.printf("Buffer: %d pixels\n", buffer_pixels);

  disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, buf1, buf2, buffer_pixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);

  M5.Display.setBrightness(200);
  ui_init();
}

void loop() {
  M5.update();

  auto t = M5.Touch.getDetail();
  is_pressed = t.isPressed();
  if (is_pressed) {
    last_x = t.x;
    last_y = t.y;
  }

  lv_timer_handler();
  delay(5);
}
