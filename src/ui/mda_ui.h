#ifndef MDA_UI_H
#define MDA_UI_H

#include <lvgl.h>

typedef struct t_mda_menu_data {
    lv_obj_t *page_home;
    lv_obj_t *page_smile;
    lv_obj_t *page_mic;
    lv_obj_t *page_settings;
}t_mda_menu_data;


/**
 * 创建整个界面：主题样式 → 页面 → 显示首页。
 * 必须在 mda_lvgl_initilaized() 之后调用一次。
 */
void ui_init();

#endif
