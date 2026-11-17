#include <M5Unified.h>
#include <lvgl.h>
#include "config/mda_config.h"
#include "mda_logger.h"

// DIRECT 模式：一整屏画布（320x240 RGB565 = 150KB），放在 PSRAM。
// LVGL 只在脏区域上画，其余像素保持上一帧；一帧所有脏区域画完后再一次性送屏，避免上下两帧拼接的撕裂。
static constexpr uint32_t FB_SIZE = MDA_SCREEN_WIDTH * MDA_SCREEN_HEIGHT * sizeof(uint16_t);

static lv_area_t s_dirty;              // 本帧所有脏区域的外接矩形
static bool s_dirty_empty = true;      // 本帧还没有记录过脏区域

static uint32_t mda_lvgl_tick_callback() {
    return millis(); //返回开机后的毫秒数，lvgl 内部计时器会用这个值计算时间差
}

/**
 * 把一块区域合并进本帧的外接矩形
 */
static void mda_lvgl_dirty_add(const lv_area_t *area) {
    if (s_dirty_empty) {
        s_dirty = *area;
        s_dirty_empty = false;
        return;
    }
    s_dirty.x1 = LV_MIN(s_dirty.x1, area->x1);
    s_dirty.y1 = LV_MIN(s_dirty.y1, area->y1);
    s_dirty.x2 = LV_MAX(s_dirty.x2, area->x2);
    s_dirty.y2 = LV_MAX(s_dirty.y2, area->y2);
}

/**
 * 刷新屏幕。DIRECT 模式下一帧有几块脏区域就会调用几次，px_map 始终指向整屏画布的起点。
 */
static void mda_lvgl_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    mda_lvgl_dirty_add(area);

    // 不是本帧最后一块：先不送屏，等这一帧全部画完
    if (!lv_display_flush_is_last(disp)) {
        lv_display_flush_ready(disp);
        return;
    }

    const int32_t x = s_dirty.x1;
    const int32_t y = s_dirty.y1;
    const int32_t w = lv_area_get_width(&s_dirty);
    const int32_t h = lv_area_get_height(&s_dirty);
    const uint16_t *fb = (const uint16_t *)px_map;

    // 画布一行是整屏宽，脏矩形每行的像素不连续，所以逐行送；setAddrWindow 之后屏幕会自动按行往下填
    M5.Display.startWrite();
    M5.Display.setAddrWindow(x, y, w, h);
    for (int32_t row = 0; row < h; row++) {
        const uint16_t *line = fb + (y + row) * MDA_SCREEN_WIDTH + x;
        M5.Display.writePixels((const lgfx::rgb565_t *)line, w);
    }
    M5.Display.endWrite();

    s_dirty_empty = true;
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
    //设置成屏幕大小 320*240
    lv_display_t *display = lv_display_create(MDA_SCREEN_WIDTH, MDA_SCREEN_HEIGHT);

    // 整屏画布放 PSRAM（内部 SRAM 放不下 150KB）；只用一块，DIRECT 双缓冲需要来回同步脏区域，同步 flush 下没有收益
    void *fb = heap_caps_malloc(FB_SIZE, MALLOC_CAP_SPIRAM);
    if (fb == nullptr) {
        logger.error("lvgl", "framebuffer alloc failed (%u bytes in PSRAM)", (unsigned)FB_SIZE);
        return;
    }
    lv_display_set_buffers(display, fb, nullptr, FB_SIZE, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, mda_lvgl_flush_callback);

    //创建一个输入设备
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, mda_touch_cb);
}

void mda_lvgl_loop() {
    lv_timer_handler();
}

