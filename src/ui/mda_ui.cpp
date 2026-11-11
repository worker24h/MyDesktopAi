#include <lvgl.h>
#include "mda_ui.h"
#include "mda_theme.h"
#include "screens/mda_screens.h"
#include "widgets/mda_statusbar.h"

void ui_init() {
  // 1. 公共样式，必须最先初始化，后面创建的控件都会用到
  theme_init();

  // 2. 创建页面（目前只有首页，其他页面在 M2 阶段补上）
  lv_obj_t *home = mda_home_create();
  lv_screen_load(home);

  // 3. 顶部状态栏：时间、电量、WiFi 等
  mda_statusbar_create();

}
