#include <string.h>
#include <lvgl.h>
#include "M5Unified.h"
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "mda_ntp.h"
#include "mda_statusbar.h"
#include "mda_network.h"


#define MAX_STATUSBAR_HEIGHT 22

// 使用icon svg
static void mda_wifi_create(lv_obj_t *parent) {
  lv_obj_t *wifi_label = lv_label_create(parent);
  lv_obj_add_style(wifi_label, &st_text, 0);                    // 默认颜色用 MDA_INK
  lv_obj_set_style_text_font(wifi_label, &font_cn_14, 0);       // 图标字体覆盖 st_text 里的字体
  lv_obj_align(wifi_label, LV_ALIGN_RIGHT_MID, -40, 0);
  if (wifi_sta_get_status().state == Connected) {
    lv_label_set_text(wifi_label, ICON_WIFI);
  } else {
    lv_label_set_text(wifi_label, ICON_WIFI_OFF);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(MDA_RED), 0);  // 断网显示红色
  }
}

#define BATTERY_REFRESH_MS 2000   // 电量刷新间隔：读 PMIC 走 I2C，没必要太频繁

static lv_obj_t *s_battery_label = nullptr;

// 根据当前电源状态选图标和颜色
static void mda_battery_update(lv_obj_t *battery_label) {
  lv_obj_set_style_text_color(battery_label, lv_color_hex(MDA_INK), 0); //默认颜色
  if (!M5.Power.Axp2101.getBatState()) {
    // 没检测到电池（拔掉/开关关闭/过放保护）时 getBatteryLevel() 返回 0，不能当成低电量
    lv_label_set_text(battery_label, ICON_PLUG);
    return;
  }
  if (M5.Power.isCharging() == m5::Power_Class::is_charging) {
    lv_label_set_text(battery_label, ICON_BATTERY_CHARGIN);
  } else {
    int battery_level = M5.Power.getBatteryLevel();
    if (battery_level >= 90) {
      lv_label_set_text(battery_label, ICON_BATTERY_FULL);
    } else if (battery_level >= 30) {
      lv_label_set_text(battery_label, ICON_BATTERY_MEDIUM);
    } else {
      lv_label_set_text(battery_label, ICON_BATTERY_LOW);
      lv_obj_set_style_text_color(battery_label, lv_color_hex(MDA_RED), 0);  // 电量低显示红色
    }
  }
}

// lv_timer 回调：由 loop() 里的 lv_timer_handler() 定时调用
static void mda_battery_timer_cb(lv_timer_t *timer) {
  // LVGL v9 里 lv_timer_t 是私有结构体，不能直接访问成员，要用访问函数
  lv_obj_t *battery_label = (lv_obj_t *)lv_timer_get_user_data(timer);
  mda_battery_update(battery_label);
  return;
}

static void mda_battery_create(lv_obj_t *parent) {
  s_battery_label = lv_label_create(parent);
  lv_obj_add_style(s_battery_label, &st_text, 0);                  // 默认颜色用 MDA_INK
  lv_obj_set_style_text_font(s_battery_label, &font_cn_18, 0);     // 电池图标偏扁，用 18px 才看得清
  lv_obj_align(s_battery_label, LV_ALIGN_RIGHT_MID, -10, 0);

  mda_battery_update(s_battery_label);                                            // 先显示一次，不用等第一个周期
  lv_timer_create(mda_battery_timer_cb, BATTERY_REFRESH_MS, s_battery_label);
}

static void mda_time_create(lv_obj_t *parent) {
  char buf[8] = {0};
  if (!mda_date_format_custom("%H:%M", buf, sizeof(buf))) {
    strncpy(buf, "--:--", 6);
  }
  lv_obj_t *time_label = lv_label_create(parent);
  lv_obj_add_style(time_label, &st_text, 0);                    // 颜色用 MDA_INK
  lv_obj_set_style_text_font(time_label, &font_num_13b, 0);     // 字体换成粗体数字，覆盖 st_text 里的字体
  lv_label_set_text(time_label, buf);
  lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 10, 0);
}

void mda_statusbar_create() {
  lv_obj_t *status_bar = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(status_bar);  // 透明容器，不要默认的白底和边框
  lv_obj_set_size(status_bar, MDA_SCREEN_WIDTH, MAX_STATUSBAR_HEIGHT);
  lv_obj_set_pos(status_bar, 0, 0);
  lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE); //不可拖动
  lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);  //不可点击

  // 创建时间、Wi-Fi、和电量图标
  mda_time_create(status_bar);
  mda_wifi_create(status_bar);
  mda_battery_create(status_bar);

}


