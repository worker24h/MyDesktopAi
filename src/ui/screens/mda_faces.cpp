
#include <lvgl.h>
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "ui/widgets/mda_robots.h"



#define FACE_HIGH    70
#define FACE_WIDTH   72
#define FACE_RADIUS  12


static void mda_face_clicked_event(lv_event_t * e) {
    mda_robot_image_info_t *robot = (mda_robot_image_info_t *)lv_event_get_user_data(e);
    lv_obj_t *face = lv_event_get_current_target_obj(e);
    uint32_t prev_actived_index = mda_robot_update(robot);
    lv_obj_t *grid = lv_obj_get_parent(face);
    lv_obj_t *prev = lv_obj_get_child(grid, prev_actived_index);
    lv_obj_set_style_border_color(prev, lv_color_hex(MDA_COLOR_WHITE), 0);
    lv_obj_set_style_border_color(face, lv_color_hex(MDA_COLOR_BLUE), 0);
}

static void mda_faces_list_create(lv_obj_t *grid) {
    uint32_t size = mda_robot_size();
    for (int i = 0; i <  size; i++) {
        lv_obj_t *face = lv_obj_create(grid);
        lv_obj_remove_style_all(face); //去掉默认样式，透明色，pad等
        lv_obj_set_size(face, FACE_WIDTH, FACE_HIGH);
        lv_obj_set_style_radius(face, FACE_RADIUS, 0);
        lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(face, lv_color_hex(MDA_COLOR_WHITE), 0);   // 设计稿 --card：白色
        lv_obj_set_style_bg_opa(face, 184, 0);                               // 72% 不透明（255 × 0.72 ≈ 184）
        lv_obj_set_style_border_color(face, lv_color_hex(MDA_COLOR_WHITE), 0);
        lv_obj_set_style_border_width(face, 2, 0);
        //列式排
        lv_obj_set_flex_flow(face, LV_FLEX_FLOW_COLUMN);
        //垂直水平居中
        lv_obj_set_flex_align(face, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        const mda_robot_image_info_t *robot = mda_get_robot(i);
        lv_obj_t *image = lv_image_create(face);
        lv_image_set_src(image, robot->image_g);

        //设置字体
        lv_obj_t *label = lv_label_create(face);
        lv_obj_set_style_text_font(label, &font_cn_11b, 0);
        lv_label_set_text(label, robot->name);

        lv_obj_add_event(face, mda_face_clicked_event, LV_EVENT_CLICKED, (void*)robot);
    }
}


lv_obj_t *mda_faces_create() {
    lv_obj_t *faces = lv_obj_create(NULL); //表示一个屏幕
    lv_obj_remove_flag(faces, LV_OBJ_FLAG_SCROLLABLE); //屏幕去掉滚动
    lv_obj_set_style_bg_opa(faces, LV_OPA_TRANSP, 0); //全透明


    //创建grid显示表情
    lv_obj_t *grid = lv_obj_create(faces);
    lv_obj_remove_style_all(grid);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER); //滑动方向
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF); //具备滚动功能，但是不要显示滚动条
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP); //flex格式，自动换行
    lv_obj_set_pos(grid, 8, 26);
    lv_obj_set_size(grid, MDA_SCREEN_WIDTH - 8 * 2, 162);
    lv_obj_set_style_pad_column(grid, 5, 0);   // 列间距5, 默认是10
    lv_obj_set_style_pad_row(grid, 5, 0);      // 行间距5， 默认是10
    mda_faces_list_create(grid);

    return faces;
}

