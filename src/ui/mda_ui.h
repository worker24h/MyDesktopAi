#ifndef MDA_UI_H
#define MDA_UI_H

#include <lvgl.h>

// 菜单栏 4 个图标对应的页面；还没实现的页面为 nullptr，点击时只忽略不切换
typedef struct mda_pages_t {
    lv_obj_t *home = nullptr;
    lv_obj_t *smile = nullptr;
    lv_obj_t *mic = nullptr;
    lv_obj_t *settings = nullptr;
}mda_pages_t;


/**
 * 创建整个界面：主题样式 → 页面 → 显示首页。
 * 必须在 mda_lvgl_initilaized() 之后调用一次。
 */
void ui_init();

#endif
