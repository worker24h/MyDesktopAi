#ifndef MDA_UI_H
#define MDA_UI_H

#include <lvgl.h>

// 菜单栏 4 个图标对应的页面；还没实现的页面为 nullptr，点击时只忽略不切换

enum mda_pages_e {
  page_home = 0,      // 主页
  page_smile = 1,     // 表情
  page_chat = 2,      // 对话
  page_settings = 3   // 设置
};

// 每个页面
typedef struct mda_page_t {
    mda_pages_e type;
    lv_obj_t *page;
    lv_obj_t *icon;
} mda_page_t;

// 所有页面
typedef struct mda_pages_t {
    mda_page_t home;
    mda_page_t smile;
    mda_page_t chat;
    mda_page_t settings;

    lv_obj_t *status_bar = nullptr;
    lv_obj_t *menu_bar = nullptr;

    mda_page_t *last_page = nullptr; // 保存上次显示的页面
} mda_pages_t;


/**
 * 创建整个界面：主题样式 → 页面 → 显示首页。
 * 必须在 mda_lvgl_initilaized() 之后调用一次。
 */
void ui_init();

#endif
