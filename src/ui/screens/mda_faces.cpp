
#include <lvgl.h>
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"

LV_IMAGE_DECLARE(robot_confused_G);
LV_IMAGE_DECLARE(robot_excited_G);
LV_IMAGE_DECLARE(robot_happy_G);
LV_IMAGE_DECLARE(robot_laugh_G);
LV_IMAGE_DECLARE(robot_listen_G);
LV_IMAGE_DECLARE(robot_playful_G);
LV_IMAGE_DECLARE(robot_sad_G);
LV_IMAGE_DECLARE(robot_sing_G);
LV_IMAGE_DECLARE(robot_sleep_G);
LV_IMAGE_DECLARE(robot_surprised_G);
LV_IMAGE_DECLARE(robot_talk_close_G);
LV_IMAGE_DECLARE(robot_talk_G);
LV_IMAGE_DECLARE(robot_think_G);


#define FACE_HIGH    70
#define FACE_WIDTH   72
#define FACE_RADIUS  12

typedef struct {
    const lv_image_dsc_t *image;   // 图片（LV_IMAGE_DECLARE 声明的是 const，这里也要 const）
    const char *name;              // 名字（字符串常量是只读的，要用 const char *）
}mda_face_image_info_t;

// 顺序和名字取自设计稿 EXPR：开心、大笑、惊讶、思考、倾听、唱歌、睡觉、难过、兴奋、调皮、疑惑、说话
static const mda_face_image_info_t face_images[] = {
    {&robot_listen_G, "倾听"},
    {&robot_happy_G, "开心"},
    {&robot_laugh_G, "大笑"},
    {&robot_surprised_G, "惊讶"},
    {&robot_think_G, "思考"},
    {&robot_sing_G, "唱歌"},
    {&robot_sleep_G, "睡觉"},
    {&robot_sad_G, "难过"},
    {&robot_excited_G, "兴奋"},
    {&robot_playful_G, "调皮"},
    {&robot_confused_G, "疑惑"},
    {&robot_talk_G, "说话"},
};

static void mda_face_clicked_event(lv_event_t * e) {
    mda_face_image_info_t *user = (mda_face_image_info_t *)lv_event_get_user_data(e);
    lv_obj_t *face = lv_event_get_current_target_obj(e);

    lv_obj_set_style_border_color(face, lv_color_hex(MDA_COLOR_BLUE), 0); 
}

static void mda_faces_list_create(lv_obj_t *grid) {
    for (int i = 0; i <  MDA_ARRAY_LEN(face_images); i++) {
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


        lv_obj_t *image = lv_image_create(face);
        lv_image_set_src(image, face_images[i].image);

        //设置字体
        lv_obj_t *label = lv_label_create(face);
        lv_obj_set_style_text_font(label, &font_cn_11b, 0);
        lv_label_set_text(label, face_images[i].name);

        lv_obj_add_event(face, mda_face_clicked_event, LV_EVENT_CLICKED, (void*)&(face_images[i]));
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

