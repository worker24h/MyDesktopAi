#include <lvgl.h>
#include "mda_screens.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"

typedef enum {
    MDA_SETTINGS_ROW_ARROW,    // 右边是 ›，点击跳转
    MDA_SETTINGS_ROW_SLIDER,   // 右边是 滑块 + 数值
} mda_settings_row_type_t;

typedef struct {
    const char *icon;
    uint32_t color;
    const char *text;
    mda_settings_row_type_t type;
    int32_t min;      // 以下 3 个只对滑块行有意义，箭头行填 0
    int32_t max;
    int32_t default_value;    // 初始值
} mda_settings_row_def_t;


static void mda_slider_change_event(lv_event_t * event) {
    lv_obj_t *value = (lv_obj_t*)lv_event_get_user_data(event);
    lv_obj_t *slider = lv_event_get_current_target_obj(event);
    int32_t v = lv_slider_get_value(slider);
    lv_label_set_text_fmt(value, "%d", v);
}

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
    lv_obj_set_style_border_color(row, lv_color_hex(MDA_COLOR_LINE), 0);

    //设置row中的内容
    lv_obj_t *icon_left = lv_obj_create(row);
    lv_obj_remove_style_all(icon_left);
    lv_obj_remove_flag(icon_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(icon_left, 22, 22);
    lv_obj_set_style_radius(icon_left, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(icon_left, lv_color_hex(param.color), 0);
    lv_obj_set_style_bg_opa(icon_left, LV_OPA_COVER, 0);
    lv_obj_t *icon = lv_label_create(icon_left);
    lv_obj_set_style_text_font(icon, &font_cn_14, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(MDA_COLOR_WHITE), 0);
    lv_label_set_text(icon, param.icon);
    lv_obj_center(icon);
    lv_obj_set_style_translate_y(icon, 2, 0); //往下挪2个像素，垂直居中

    lv_obj_t *text = lv_label_create(row);
    lv_obj_add_style(text, &st_text, 0);
    lv_obj_set_style_text_font(text, &font_cn_11b, 0);
    lv_label_set_text(text, param.text);
    lv_obj_set_flex_grow(text, 1);

    if (param.type == MDA_SETTINGS_ROW_ARROW) {// >箭头
        lv_obj_t *icon_arrow = lv_label_create(row);
        lv_label_set_text(icon_arrow, ICON_RIGHT);
        lv_obj_set_style_text_font(icon_arrow, &font_cn_11, 0);
        lv_obj_set_style_bg_color(icon_arrow, lv_color_hex(0x9AA0BF), 0);
    } else {// 滑块
        lv_obj_t *slider = lv_slider_create(row);
        lv_obj_set_size(slider, 72, 4);
        lv_slider_set_range(slider, param.min, param.max);
        lv_slider_set_value(slider, param.default_value, LV_ANIM_ON);
        // 滑块圆钮
        lv_obj_set_style_bg_color(slider, lv_color_hex(MDA_COLOR_VIOLET), LV_PART_INDICATOR);
        lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB);
        lv_obj_set_style_border_width(slider, 1, LV_PART_KNOB);
        lv_obj_set_style_border_color(slider, lv_color_hex(MDA_COLOR_VIOLET), LV_PART_KNOB);
        // 手指很难按准，四周扩大 10px
        lv_obj_set_ext_click_area(slider, 10);
        
        lv_obj_t *value = lv_label_create(row);
        lv_label_set_text_fmt(value, "%d", param.default_value);
        lv_obj_set_style_text_font(value, &font_cn_11b, 0);
        lv_obj_set_style_text_color(value, lv_color_hex(MDA_COLOR_INK2), 0);
        lv_obj_set_width(value, 20);
        lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);

        lv_obj_add_event_cb(slider, mda_slider_change_event, LV_EVENT_VALUE_CHANGED, value);

    }
    
    return row;
}

static void mda_settings_list_create(lv_obj_t *parent) {
    constexpr int max_size = 5;
    static const mda_settings_row_def_t controls[max_size] = {
        {ICON_SMILE, MDA_COLOR_YELLOW, "角色与表情", MDA_SETTINGS_ROW_ARROW, 0, 0, 0},
        {ICON_VOLUME, MDA_COLOR_BLUE, "音量", MDA_SETTINGS_ROW_SLIDER, 0, 100, 50},
        {ICON_SUN, MDA_COLOR_VIOLET, "亮度", MDA_SETTINGS_ROW_SLIDER, 10, 100, 50},
        {ICON_WIFI, MDA_COLOR_OK, "网络与连接", MDA_SETTINGS_ROW_ARROW, 0, 0, 0},
        {ICON_INFO, MDA_COLOR_GREEN, "关于设备", MDA_SETTINGS_ROW_ARROW, 0, 0, 0}
    };

    for (int i = 0; i < max_size; i++) {
        lv_obj_t *row = mda_settings_row_create(parent, controls[i]);
        if (i == max_size - 1) {//最后一行不设置，border=1
            lv_obj_set_style_border_width(row, 0, 0);
        }
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

    mda_settings_list_create(card);

    return settings;
}

