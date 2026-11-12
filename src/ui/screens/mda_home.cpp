// 首页：左边问候气泡 + 机器人，右边 2×2 入口块（设计稿 5.1 节）
#include <lvgl.h>
#include "mda_screens.h"
#include "../mda_theme.h"
#include "assets/fonts/mda_fonts.h"

//声明图片
LV_IMAGE_DECLARE(bg_main);
LV_IMAGE_DECLARE(robot_happy_L);

// 入口块：底色、图标渐变、图标、文字
struct TileDef {
  uint32_t bg;
  lv_style_t *grad;
  const char *icon;
  const char *text;
};

/**
 * 去掉 LVGL 默认的滚动，否则容器可以被拖动
 */
static void no_scroll(lv_obj_t *obj) {
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

/**
 * 左列：问候气泡 + 机器人，竖向排列、水平居中（设计稿 .home-l：x6，宽 146）
 */
static void create_left(lv_obj_t *scr) {
  lv_obj_t *col = lv_obj_create(scr);
  lv_obj_remove_style_all(col);  // 透明容器，不要默认的白底和边框
  lv_obj_set_pos(col, 6, 26);
  lv_obj_set_size(col, 146, 172);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(col, 2, 0);
  no_scroll(col);

  // 气泡：宽度跟着文字走，最大 142
  lv_obj_t *bubble = lv_obj_create(col);
  lv_obj_add_style(bubble, &st_card_strong, 0);
  lv_obj_set_size(bubble, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_max_width(bubble, 142, 0);
  lv_obj_set_style_pad_hor(bubble, 10, 0);
  lv_obj_set_style_pad_ver(bubble, 6, 0);
  no_scroll(bubble);

  lv_obj_t *greet = lv_label_create(bubble);
  lv_obj_add_style(greet, &st_text, 0);
  lv_obj_set_style_text_font(greet, &font_cn_11b, 0);
  lv_obj_set_style_text_align(greet, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_line_space(greet, 3, 0);
  lv_label_set_text(greet, "你好呀！\n今天想和我玩什么？");

  // 机器人
  lv_obj_t *robot = lv_image_create(col);
  lv_image_set_src(robot, &robot_happy_L);
}

/**
 * 一个入口块：图标方块在上，文字在下，竖向居中（设计稿 .tile）
 */
static lv_obj_t *create_tile(lv_obj_t *parent, const TileDef &def) {
  lv_obj_t *tile = lv_button_create(parent);
  lv_obj_remove_style_all(tile);  // 去掉按钮默认的蓝色主题
  lv_obj_add_style(tile, &st_tile, 0);
  lv_obj_set_style_bg_color(tile, lv_color_hex(def.bg), 0);
  lv_obj_set_size(tile, 73, 62);
  lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(tile, 4, 0);

  // 按下时缩小到 94%，以中心为缩放原点（设计稿 .tile:active{transform:scale(.94)}）
  lv_obj_set_style_transform_scale(tile, 240, LV_STATE_PRESSED);
  lv_obj_set_style_transform_pivot_x(tile, 73 / 2, 0);
  lv_obj_set_style_transform_pivot_y(tile, 62 / 2, 0);

  // 图标方块：28×28，图标居中
  lv_obj_t *box = lv_obj_create(tile);
  lv_obj_add_style(box, &st_icon_box, 0);
  lv_obj_add_style(box, def.grad, 0);
  lv_obj_set_size(box, 28, 28);
  lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);  // 让点击落到入口块上
  no_scroll(box);

  lv_obj_t *icon = lv_label_create(box);
  lv_obj_set_style_text_font(icon, &font_cn_14, 0);
  lv_label_set_text(icon, def.icon);
  lv_obj_center(icon);

  lv_obj_t *label = lv_label_create(tile);
  lv_obj_set_style_text_font(label, &font_cn_11b, 0);
  lv_label_set_text(label, def.text);

  return tile;
}

/**
 * 右侧 2×2 入口块（设计稿 .tiles：x158，y30，每块 73×62，间距 7）
 */
static void create_tiles(lv_obj_t *scr) {
  static const TileDef TILES[] = {
    {MDA_TILE_PINK, &st_grad_pink, ICON_MUSIC, "唱歌故事"},
    {MDA_TILE_BLUE, &st_grad_blue, ICON_CHAT, "AI 聊天"},
    {MDA_TILE_GREEN, &st_grad_green, ICON_BOT, "机器人控制"},
    {MDA_TILE_VIOLET, &st_grad_violet, ICON_CASE, "设备管理"},
  };
  constexpr int X0 = 158, Y0 = 30, W = 73, H = 62, GAP = 7;

  for (int i = 0; i < 4; i++) {
    lv_obj_t *tile = create_tile(scr, TILES[i]);
    lv_obj_set_pos(tile, X0 + (i % 2) * (W + GAP), Y0 + (i / 2) * (H + GAP));
  }
}

lv_obj_t *mda_home_create() {
  lv_obj_t *scr = lv_obj_create(NULL);
  no_scroll(scr);

  // 背景图铺满全屏
  lv_obj_t *bg = lv_image_create(scr);
  lv_image_set_src(bg, &bg_main);
  lv_obj_set_pos(bg, 0, 0);

  create_left(scr);
  create_tiles(scr);
  return scr;
}
