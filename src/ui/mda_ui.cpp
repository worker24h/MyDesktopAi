#include <lvgl.h>
#include "mda_ui.h"
#include "mda_theme.h"
#include "screens/mda_screens.h"

void ui_init() {
  // 1. 公共样式，必须最先初始化，后面创建的控件都会用到
  theme_init();

  // 2. 创建页面（目前只有首页，其他页面在 M2 阶段补上）
  lv_obj_t *home = scr_home_create();

  // 3. 显示首页
  lv_screen_load(home);
}
