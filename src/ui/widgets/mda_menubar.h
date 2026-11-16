#ifndef MDA_MENUBAR_H
#define MDA_MENUBAR_H

#include <lvgl.h>
#include "ui/mda_ui.h"

// pages 必须一直有效（传静态变量的地址），点击图标时会读取里面的页面
lv_obj_t *mda_menubar_create(mda_pages_t *pages);


#endif
