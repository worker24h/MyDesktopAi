#include <M5Unified.h>
#include <lvgl.h>

static constexpr int W = 320, H = 240;
static constexpr int BUF_LINES = 40;  // 每次刷新 40 行，缓冲区大小 = W * BUF_LINES * 2（RGB565）

static uint32_t mda_lvgl_tick_callback() {
  return millis(); //返回开机后的毫秒数，lvgl 内部计时器会用这个值计算时间差
}

/**
 * 刷新屏幕
 */
static void mda_lvgl_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  //长宽
    const uint32_t w = lv_area_get_width(area);
  const uint32_t h = lv_area_get_height(area);

  //开始写入
  M5.Display.startWrite();
  M5.Display.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t *)px_map);
  M5.Display.endWrite();
  
  //告诉 lvgl 刷新完成
  lv_display_flush_ready(disp);
}

/**
 * 读取触摸屏状态
 */
static void mda_touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  auto t = M5.Touch.getDetail();
  if (t.isPressed()) {
    data->point.x = t.x;
    data->point.y = t.y;
    data->state = LV_INDEV_STATE_PRESSED; //按下
  } else {
    data->state = LV_INDEV_STATE_RELEASED; //放开
  }
}

/**
 * 初始化 LVGL。必须在 M5.begin() 之后调用，M5.update() 之前调用。
 */
void mda_lvgl_initilaized() {
  lv_init();
  lv_tick_set_cb(mda_lvgl_tick_callback);
  //设置成屏幕大小
  lv_display_t *display = lv_display_create(W, H);

  // RGB565
  const size_t sz = W * BUF_LINES * sizeof(uint16_t);
  void *buf2 = heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  void *buf1 = heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_display_set_buffers(display, buf1, buf2, sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, mda_lvgl_flush_callback);

  //创建一个输入设备
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, mda_touch_cb);
}

void mda_lvgl_loop() {
  lv_timer_handler();
}

