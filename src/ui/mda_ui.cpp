#include <lvgl.h>
#include "mda_ui.h"
#include "mda_theme.h"
#include "screens/mda_screens.h"
#include "widgets/mda_statusbar.h"
#include "widgets/mda_menubar.h"
#include "widgets/mda_robots.h"
#include "assets/images/mda_images.h"

static mda_pages_t s_pages;

void ui_init() {
    // 1. 公共样式，必须最先初始化，后面创建的控件都会用到
    theme_init();
    mda_robot_initalize();

    // 2. 背景图铺满全屏
    lv_obj_t *bg = lv_image_create(lv_layer_bottom());
    lv_image_set_src(bg, &bg_main);
    lv_obj_set_pos(bg, 0, 0);

    // 3. 顶部状态栏：时间、电量、WiFi 等
    lv_obj_t *status_bar = mda_statusbar_create();
    s_pages.status_bar = status_bar;
    
    // 4. 创建页面
    s_pages.home = mda_home_create();
    s_pages.smile = mda_faces_create();
    s_pages.mic = mda_chat_create();
    s_pages.settings = mda_settings_create();
    lv_screen_load(s_pages.home);

    // 5. 底部菜单栏：放在 lv_layer_top，所有根页面共用
    s_pages.menu_bar = mda_menubar_create(&s_pages);
}

