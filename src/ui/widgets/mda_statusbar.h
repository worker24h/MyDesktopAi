#ifndef MDA_STATUSBAR_H
#define MDA_STATUSBAR_H

#include <lvgl.h>
#include "ui/mda_ui.h"

// 状态栏左上角互斥模式
typedef enum {
    MDA_STATUSBAR_ROOT,    // 根页面：显示时间，隐藏返回区
    MDA_STATUSBAR_SUBPAGE, // 二级页面：隐藏时间，显示 [< 标题]
} mda_statusbar_mode_t;

lv_obj_t *mda_statusbar_create(const mda_pages_t *pages);

// 切换模式：SUBPAGE 时传入标题字符串，ROOT 时 title 传 nullptr
void mda_statusbar_set_mode(mda_statusbar_mode_t mode, const char *title = nullptr);

#endif
