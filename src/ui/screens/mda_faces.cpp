
#include <lvgl.h>
#include "config/mda_config.h"
#include "ui/mda_theme.h"

LV_IMAGE_DECLARE(robot_happy_G);
LV_IMAGE_DECLARE(robot_confused_blink_G);
LV_IMAGE_DECLARE(robot_confused_blink_S);

#define FACE_HIGH 70
static void mda_faces_list_create(lv_obj_t *grid) {
    for (int i = 0; i < 12; i++) {
        lv_obj_t *face = lv_obj_create(grid);
        lv_obj_set_size(face, 80, 130);
        lv_obj_set_style_radius(face, 12, 0);
        lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(face, lv_color_hex(MDA_COLOR_RED), 0);
        lv_obj_set_style_border_color(face, lv_color_hex(MDA_COLOR_WHITE), 0);
        lv_obj_set_style_border_width(face, 2, 0);

        lv_obj_t *image = lv_image_create(face);
        lv_image_set_src(image, &robot_confused_blink_G);
        lv_obj_center(image);
        lv_obj_t *label = lv_label_create(face);
        lv_label_set_text(label, "hello");
    }
}


lv_obj_t *mda_faces_create() {
    lv_obj_t *faces = lv_obj_create(NULL); //表示一个屏幕
    lv_obj_remove_flag(faces, LV_OBJ_FLAG_SCROLLABLE); //屏幕去掉滚动
    lv_obj_set_style_bg_opa(faces, LV_OPA_TRANSP, 0); //全透明

    
    //创建grid显示表情
    lv_obj_t *grid = lv_obj_create(faces);
    lv_obj_remove_style_all(grid);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF); //具备滚动功能，但是不要显示滚动条
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP); //flex格式，自动换行
    lv_obj_set_pos(grid, 8, 26);
    lv_obj_set_size(grid, MDA_SCREEN_WIDTH - 8 * 2, FACE_HIGH * 3);
    lv_obj_set_style_pad_column(grid, 5, 0);   // 列间距5, 默认是10
    lv_obj_set_style_pad_row(grid, 5, 0);      // 行间距5， 默认是10
    mda_faces_list_create(grid);

    return faces;
}

