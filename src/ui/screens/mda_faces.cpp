#include <stdint.h>
#include <lvgl.h>
#include "mda_screens.h"
#include "ui/mda_theme.h"
#include "ui/mda_expr.h"
#include "assets/fonts/mda_fonts.h"

// 网格：4 列 × 3 行，x8–312，y26 起，每格约 46×70，间距 5（设计稿 .egrid）
#define FACES_COLS 4
#define FACES_CELL_H 70
#define FACES_GAP 5

static lv_style_t st_face_on;                  // 选中样式：实白底 + 紫色描边，只在 LV_STATE_CHECKED 时生效
static lv_obj_t *s_checked_cell = nullptr;     // 当前选中的格子

/**
 * 点击一格：选中框移过去，并记为当前表情
 */
static void mda_faces_cell_clicked_cb(lv_event_t *e) {
    lv_obj_t *cell = lv_event_get_current_target_obj(e);
    mda_expr_t expr = (mda_expr_t)(intptr_t)lv_event_get_user_data(e);
    if (cell == s_checked_cell) {
        return;    // 点的就是当前表情
    }

    if (s_checked_cell != nullptr) {
        lv_obj_remove_state(s_checked_cell, LV_STATE_CHECKED);
    }
    lv_obj_add_state(cell, LV_STATE_CHECKED);    // 样式由 st_face_on 自动套上，不用手动改颜色
    s_checked_cell = cell;

    mda_expr_set_current(expr);
}

/**
 * 一格：卡片 + S 尺寸机器人 + 名字，竖排居中
 */
static lv_obj_t *mda_faces_cell_create(lv_obj_t *parent, mda_expr_t expr) {
    const mda_expr_info_t *info = mda_expr_info(expr);

    lv_obj_t *cell = lv_obj_create(parent);
    lv_obj_remove_style_all(cell);
    lv_obj_add_style(cell, &st_card, 0);                        // 平时：半透明白卡片
    lv_obj_add_style(cell, &st_face_on, LV_STATE_CHECKED);      // 选中时叠加：实白底 + 紫边
    lv_obj_set_style_radius(cell, 12, 0);
    lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cell, 2, 0);

    // 图片和 label 默认不可点击，点击会落到格子上
    lv_obj_t *img = lv_image_create(cell);
    lv_image_set_src(img, info->img_s);

    lv_obj_t *label = lv_label_create(cell);
    lv_obj_add_style(label, &st_text, 0);
    lv_obj_set_style_text_font(label, &font_cn_11b, 0);      // 设计稿 10px 粗体，最接近的是 11px
    lv_label_set_text(label, info->name);

    lv_obj_add_event_cb(cell, mda_faces_cell_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)expr);
    return cell;
}

/**
 * 4×3 网格：用 grid 布局，6 列平分宽度，格子自动填满单元格
 */
static void mda_faces_grid_create(lv_obj_t *parent) {
    // 必须是 static：LVGL 只记住数组指针，不复制
    static const int32_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST,
    };
    static const int32_t row_dsc[] = {FACES_CELL_H, FACES_CELL_H, LV_GRID_TEMPLATE_LAST};

    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_remove_style_all(grid);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(grid, 8, 26);
    lv_obj_set_size(grid, 304, FACES_CELL_H * 3 + FACES_GAP);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, FACES_GAP, 0);
    lv_obj_set_style_pad_row(grid, FACES_GAP, 0);

    mda_expr_t current = mda_expr_current();
    for (int i = 0; i < MDA_EXPR_COUNT; i++) {
        lv_obj_t *cell = mda_faces_cell_create(grid, (mda_expr_t)i);
        lv_obj_set_grid_cell(cell,
                             LV_GRID_ALIGN_STRETCH, i % FACES_COLS, 1,     // 第 i%6 列，占 1 列
                             LV_GRID_ALIGN_STRETCH, i / FACES_COLS, 1);    // 第 i/6 行，占 1 行
        if (i == current) {
            lv_obj_add_state(cell, LV_STATE_CHECKED);    // 开机时选中当前表情
            s_checked_cell = cell;
        }
    }
}

lv_obj_t *mda_faces_create() {
    // 选中样式只初始化一次（页面只创建一次）
    lv_style_init(&st_face_on);
    lv_style_set_bg_color(&st_face_on, lv_color_white());
    lv_style_set_bg_opa(&st_face_on, LV_OPA_COVER);
    lv_style_set_border_width(&st_face_on, 2);       // 设计稿 1.5px，LVGL 只能整数
    lv_style_set_border_color(&st_face_on, lv_color_hex(MDA_COLOR_VIOLET));

    lv_obj_t *faces = lv_obj_create(NULL);
    lv_obj_remove_flag(faces, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(faces, LV_OPA_TRANSP, 0);    // 透出 bottom 层的背景图

    mda_faces_grid_create(faces);
    return faces;
}
