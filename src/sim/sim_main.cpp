/**
 * 电脑仿真入口：只在 env:sim（定义了 MDA_SIM）里编译。
 * ESP32 固件用的是 src/main.cpp 里的 setup()/loop()，这个文件在那边是空的。
 */
#ifdef MDA_SIM

#include <unistd.h>
#include <lvgl.h>
#include LV_SDL_INCLUDE_PATH
#include "config/mda_config.h"
#include "ui/mda_ui.h"

static constexpr int WINDOW_ZOOM = 1;    // 窗口放大倍数：1 = 窗口就是 320x240，和真机屏幕一样大

int main() {
    lv_init();

    // 放大时用最近邻采样（像素直接复制成方块），不做线性插值，避免文字和图标发虚
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    // SDL 窗口当作屏幕；不设 tick 回调时 LVGL 会自动用 SDL_GetTicks 计时
    lv_display_t *display = lv_sdl_window_create(MDA_SCREEN_WIDTH, MDA_SCREEN_HEIGHT);
    lv_sdl_window_set_zoom(display, WINDOW_ZOOM);

    // 鼠标当作触摸屏：按下左键 = 手指按下
    lv_sdl_mouse_create();

    ui_init();

    // 对应真机 loop() 里的 lv_timer_handler()；关闭窗口时 SDL 驱动会直接退出程序
    while (true) {
        uint32_t idle_ms = lv_timer_handler();
        if (idle_ms > 10) {
            idle_ms = 10;
        }
        usleep(idle_ms * 1000);
    }
    return 0;
}

#endif
