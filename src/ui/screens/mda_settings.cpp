#include <lvgl.h>
#include "mda_screens.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"

lv_obj_t *mda_settings_create() {
    lv_obj_t *settings = lv_obj_create(NULL);
    lv_obj_remove_flag(settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(settings, LV_OPA_TRANSP, 0);

    //测试
    lv_obj_t *label = lv_label_create(settings);
    lv_obj_add_style(label, &st_title, 0);
    lv_label_set_text(label, "Test");
    lv_obj_center(label);

    return settings;
}
