
#include "mda_menubar.h"
#include "../mda_theme.h"
#include "assets/fonts/mda_fonts.h"

/**
 * 创建一个菜单图标。位置由父容器的 flex 布局决定，这里不用 lv_obj_align
 */
static lv_obj_t *mda_menubar_icon_create(lv_obj_t *parent, const char *icon) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, icon);
    // 纯图标字体：行高等于字号，没有中文字体基线下方的空白，flex 居中后图标就在正中间
    lv_obj_set_style_text_font(label, &font_icon_24, 0);
    return label;
}

void mda_menubar_create(lv_obj_t *parent) {
    lv_obj_t *menubar = lv_obj_create(parent);
    lv_obj_remove_style_all(menubar);                        // 去掉默认主题的白底、边框、内边距
    lv_obj_add_style(menubar, &st_card_strong, 0);
    lv_obj_set_style_bg_opa(menubar, LV_OPA_40, 0);          // 背景不透明度 40%（0~255，可用 LV_OPA_0..LV_OPA_100）
    lv_obj_set_size(menubar, 296, 40);
    lv_obj_set_style_radius(menubar, 20, 0);
    lv_obj_remove_flag(menubar, LV_OBJ_FLAG_SCROLLABLE);     // 不可拖动
    lv_obj_align(menubar, LV_ALIGN_BOTTOM_MID, 0, -8);       // 底部居中，向上偏移 8px

    // flex 横向排列：图标之间、两端留出相同的间距；交叉轴（竖直）居中
    lv_obj_set_flex_flow(menubar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(menubar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *home = mda_menubar_icon_create(menubar, ICON_HOME);
    lv_obj_set_style_text_color(home, lv_color_hex(MDA_PINK), 0);  // 当前页高亮

    mda_menubar_icon_create(menubar, ICON_SMILE);
    mda_menubar_icon_create(menubar, ICON_MIC);
    mda_menubar_icon_create(menubar, ICON_SETTINGS);
}

