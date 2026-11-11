#include <string.h>
#include <lvgl.h>
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "mda_ntp.h"
#include "mda_statusbar.h"


#define MAX_STATUSBAR_HEIGHT 22
void mda_statusbar_create() {
  lv_obj_t *status_bar = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(status_bar);  // 透明容器，不要默认的白底和边框
  lv_obj_set_size(status_bar, MDA_SCREEN_WIDTH, MAX_STATUSBAR_HEIGHT);
  lv_obj_set_pos(status_bar, 0, 0);
  lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE); //不可拖动
  lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);  //不可点击

  //调试代码，半透明红色背景
//   lv_obj_set_style_bg_color(status_bar, lv_palette_main(LV_PALETTE_RED), 0);
//   lv_obj_set_style_bg_opa(status_bar, LV_OPA_50, 0);

  char buf[8] = {0};
  if (!mda_date_format_custom("%H:%M", buf, sizeof(buf))) {
    strncpy(buf, "00:00", 6);
  }
  lv_obj_t *time_label = lv_label_create(status_bar);
  lv_obj_add_style(time_label, &st_text, 0);                    // 颜色用 MDA_INK
  lv_obj_set_style_text_font(time_label, &font_num_13b, 0);     // 字体换成粗体数字，覆盖 st_text 里的字体
  lv_label_set_text(time_label, buf);
  lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 10, 0);
}

