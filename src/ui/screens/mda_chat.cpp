
#include <lvgl.h>
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "ui/widgets/mda_robots.h"


lv_obj_t *mda_chat_create() {
    lv_obj_t *chat = lv_obj_create(NULL); //表示一个屏幕
    lv_obj_remove_flag(chat, LV_OBJ_FLAG_SCROLLABLE); //屏幕去掉滚动
    lv_obj_set_style_bg_opa(chat, LV_OPA_TRANSP, 0); //全透明


    return chat;
}

