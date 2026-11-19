#include <lvgl.h>
#include "mda_screens.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"

typedef struct {
    const char *icon;
    uint32_t color;
    const char *text;
} mda_settings_row_def_t;

static lv_obj_t *mda_settings_row_create(lv_obj_t *parent, const mda_settings_row_def_t &param) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 31); // 100%宽， 31px的高
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW); //横排
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(MDA_LINE), 0);

    //设置row中的内容
    lv_obj_t *icon_left = lv_obj_create(row);
    lv_obj_remove_style_all(icon_left);
    lv_obj_set_style_radius(icon_left, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(icon_left, lv_color_hex(param.color), 0);
    lv_obj_set_style_bg_opa(icon_left, LV_OPA_COVER, 0);
    lv_obj_t *icon = lv_label_create(icon_left);
    lv_obj_set_style_text_font(icon, &font_cn_11b, 0);
    lv_label_set_text(icon, param.icon);
    lv_obj_center(icon);

    lv_obj_t *text = lv_label_create(row);
    lv_obj_add_style(text, &st_text, 0);
    lv_obj_set_style_text_font(text, &font_cn_11b, 0);
    lv_label_set_text(text, param.text);
    lv_obj_set_flex_grow(text, 1);

    // >箭头
    lv_obj_t *icon_arrow = lv_label_create(row);
    lv_obj_set_style_text_font(icon_arrow, &font_cn_11, 0);
    lv_obj_set_style_bg_color(icon_arrow, lv_color_hex(param.color), 0);

    return row;
}

static void mda_control_panel(lv_obj_t *parent) {
    static const mda_settings_row_def_t controls[] = {
        {ICON_SMILE, MDA_YELLOW, "角色与表情"},
        {ICON_VOLUME, MDA_BLUE, "音量"},
        {ICON_SUN, MDA_VIOLET, "亮度"},
        {ICON_WIFI, MDA_OK, "网络与链接"},
        {ICON_INFO, MDA_GREEN, "关于设备"}
    };

    for (int i = 0; i < 5; i++) {
        mda_settings_row_create(parent, controls[i]);
    }
}


lv_obj_t *mda_settings_create() {
    lv_obj_t *settings = lv_obj_create(NULL);
    lv_obj_remove_flag(settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(settings, LV_OPA_TRANSP, 0);

    //右侧
    lv_obj_t *card = lv_obj_create(settings);
    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, &st_card, 0);
    lv_obj_set_pos(card, 114, 26);
    lv_obj_set_size(card, 198, 155);
    lv_obj_set_style_pad_hor(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN); // 使用flow，列排列
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    mda_control_panel(card);

    return settings;
}

