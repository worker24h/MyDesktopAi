#include <string.h>
#include <lvgl.h>
#include "hal/mda_device.h"
#include "mda_screens.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "ui/widgets/mda_robots.h"

typedef enum {
    MDA_SLIDER_SPEADKER, 
    MDA_SLIDER_DISPLAYER,
} mda_slider_type_t;
typedef enum {
    MDA_SETTINGS_ROW_ARROW,    // 右边是 ›，点击跳转
    MDA_SETTINGS_ROW_SLIDER,   // 右边是 滑块 + 数值
} mda_settings_row_type_t;

#define ROW_ROLE_FACE_ID     "row_role_face"
#define ROW_VOLUME_ID        "row_volume"
#define ROW_DISPLAY_LIGHT_ID "row_display_light"
#define ROW_NETWORK_LINKE_ID "row_network_link"
#define ROW_ABOUT_ID         "row_about"

typedef struct {
    const char *id;
    const char *icon;
    uint32_t color;
    const char *text;
    mda_settings_row_type_t type;
    int32_t min;      // 以下 3 个只对滑块行有意义，箭头行填 0
    int32_t max;
    int32_t default_value;    // 初始值
} mda_settings_row_def_t;

typedef struct {
    lv_obj_t *label_text;
    mda_slider_type_t slider_type;
} mda_slider_user_data_t;

// 声音/屏幕亮度最大值是255，但是页面设置上最大值是100，因此需要对应一下
// 例：0 → 0，50 → 128，60 → 153，80 → 204，100 → 255
static void mda_slider_change_event(lv_event_t * event) {
    lv_obj_t *label_text = (lv_obj_t*)lv_event_get_user_data(event);
    lv_obj_t *slider = lv_event_get_current_target_obj(event);
    int32_t v = lv_slider_get_value(slider);
    lv_label_set_text_fmt(label_text, "%" LV_PRId32, v);
    const char* id = (const char*)lv_obj_get_user_data(slider);
    if (id && strcmp(id, ROW_VOLUME_ID) == 0) {
        mda_device_set_volume((uint8_t)v);
    } else {
        mda_device_set_brightness((uint8_t)v);
    }
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
        lv_obj_set_user_data(slider, (void*)param.id);
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
        lv_label_set_text_fmt(value, "%" LV_PRId32, param.default_value);
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
        {ROW_ROLE_FACE_ID, ICON_SMILE, MDA_COLOR_YELLOW, "角色与表情", MDA_SETTINGS_ROW_ARROW, 0, 0, 0},
        {ROW_VOLUME_ID, ICON_VOLUME, MDA_COLOR_BLUE, "音量", MDA_SETTINGS_ROW_SLIDER, 0, 100, 50},
        {ROW_DISPLAY_LIGHT_ID, ICON_SUN, MDA_COLOR_VIOLET, "亮度", MDA_SETTINGS_ROW_SLIDER, 10, 100, 50},
        {ROW_NETWORK_LINKE_ID, ICON_WIFI, MDA_COLOR_OK, "网络与连接", MDA_SETTINGS_ROW_ARROW, 0, 0, 0},
        {ROW_ABOUT_ID, ICON_INFO, MDA_COLOR_GREEN, "关于设备", MDA_SETTINGS_ROW_ARROW, 0, 0, 0}
    };

    for (int i = 0; i < max_size; i++) {
        lv_obj_t *row = mda_settings_row_create(parent, controls[i]);
        if (i == max_size - 1) {//最后一行不设置，border=1
            lv_obj_set_style_border_width(row, 0, 0);
        }
    }
}

//订阅，回调函数
static void mda_robot_observer_cb(lv_observer_t * observer, lv_subject_t * subject) {
    // 机器人
    lv_obj_t *robot = (lv_obj_t *)lv_observer_get_user_data(observer);
    int32_t active_id = lv_subject_get_int(subject);
    const mda_robot_image_info_t *actived_robot = mda_get_robot(active_id);
    lv_image_set_src(robot, actived_robot->image_m);
}

lv_obj_t *mda_settings_create() {
    lv_obj_t *settings = lv_obj_create(NULL);
    lv_obj_remove_flag(settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(settings, LV_OPA_TRANSP, 0);

    //右侧
    lv_obj_t *right_card = lv_obj_create(settings);
    lv_obj_remove_style_all(right_card);
    lv_obj_add_style(right_card, &st_card, 0);
    lv_obj_set_pos(right_card, 114, 26);
    lv_obj_set_size(right_card, 198, 155);
    lv_obj_set_style_pad_hor(right_card, 10, 0);
    lv_obj_set_flex_flow(right_card, LV_FLEX_FLOW_COLUMN); // 使用flow，列排列
    lv_obj_remove_flag(right_card, LV_OBJ_FLAG_SCROLLABLE);
    mda_settings_list_create(right_card);

    //左侧
    lv_obj_t *left_robot = lv_obj_create(settings);
    lv_obj_remove_style_all(left_robot);
    lv_obj_remove_flag(left_robot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(left_robot, 8, 28);
    lv_obj_set_size(left_robot, 100, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left_robot, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_robot, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(left_robot, 4, 0);

    lv_obj_t *avatar = lv_obj_create(left_robot);
    lv_obj_remove_style_all(avatar);
    lv_obj_remove_flag(avatar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(avatar, 90, 90);
    lv_obj_set_style_radius(avatar, LV_RADIUS_CIRCLE, 0);
    //渐变
    lv_obj_set_style_bg_color(avatar, lv_color_hex(MDA_COLOR_WHITE), 0);
    lv_obj_set_style_bg_grad_color(avatar, lv_color_hex(0xDCECFF), 0); //渐变色，浅蓝色
    lv_obj_set_style_bg_grad_dir(avatar, LV_GRAD_DIR_VER, 0); // 从上往下渐变，最终是由白色到浅蓝色
    lv_obj_set_style_bg_opa(avatar, LV_OPA_COVER, 0); //透明度255，不透明

    //border
    lv_obj_set_style_border_color(avatar, lv_color_hex(MDA_COLOR_WHITE), 0);
    lv_obj_set_style_border_width(avatar, 2, 0);

    lv_obj_t *image = lv_image_create(avatar);
    const mda_robot_image_info_t *robot = mda_get_actived_robot();
    lv_image_set_src(image, robot->image_m); // M 尺寸 80x69，放在 90px 的圆里
    mda_robot_subscriber(mda_robot_observer_cb, image);  
    lv_obj_center(image);

    // 名字：设计稿 13px 粗体，项目里没有这个字号，用最接近的 14px
    lv_obj_t *s_name_label = lv_label_create(left_robot);
    lv_obj_add_style(s_name_label, &st_text, 0);                  // 颜色 MDA_COLOR_INK
    lv_obj_set_style_text_font(s_name_label, &font_cn_14, 0);
    lv_label_set_text(s_name_label, "汤圆");

    // 签名：11px 灰色（st_text2）
    lv_obj_t *sign_label = lv_label_create(left_robot);
    lv_obj_add_style(sign_label, &st_text2, 0);
    lv_label_set_text(sign_label, "陪伴每一天～");

    return settings;
}

