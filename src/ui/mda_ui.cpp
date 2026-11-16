#include <lvgl.h>
#include "mda_ui.h"
#include "mda_theme.h"
#include "screens/mda_screens.h"
#include "widgets/mda_statusbar.h"
#include "widgets/mda_menubar.h"

// 声明背景图片
LV_IMAGE_DECLARE(bg_main);

static t_mda_menu_data menu_data;

void ui_init() {
    // 1. 公共样式，必须最先初始化，后面创建的控件都会用到
    theme_init();

    // 2. 背景图铺满全屏
    lv_obj_t *bg = lv_image_create(lv_layer_bottom());
    lv_image_set_src(bg, &bg_main);
    lv_obj_set_pos(bg, 0, 0);

    // 3. 顶部状态栏：时间、电量、WiFi 等
    mda_statusbar_create();

    // 4. 创建页面（目前只有首页，其他页面在 M2 阶段补上）
    lv_obj_t *home = mda_home_create();
    lv_screen_load(home);
    menu_data.page_home = home;
    
    lv_obj_t *settings = mda_settings_create();
    menu_data.page_settings = settings;
    
    // 5. 底部菜单栏：放在 lv_layer_top，所有根页面共用
    lv_obj_t *menu = mda_menubar_create(&menu_data);
}

