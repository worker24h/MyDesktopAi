#ifndef MDA_SCREENS_H
#define MDA_SCREENS_H

#include <lvgl.h>
#include "ui/mda_ui.h"

// 每个页面一个创建函数，返回新建的屏幕对象（lv_obj_create(NULL)）
void mda_home_create(mda_page_t& home);
void mda_settings_create(mda_page_t& settings);
void mda_smile_create(mda_page_t& smile);
void mda_chat_create(mda_page_t& chat);

#endif
