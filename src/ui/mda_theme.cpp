#include "mda_theme.h"
#include "assets/fonts/mda_fonts.h"

lv_style_t st_card, st_card_strong, st_tile, st_icon_box, st_text, st_text2, st_title;
lv_style_t st_grad_pink, st_grad_blue, st_grad_green, st_grad_violet;

// CSS 里的透明度是 0~1，LVGL 是 0~255。编译时换算，例如 opa(72) = 183
static constexpr lv_opa_t opa(int percent) {
  return (lv_opa_t)(percent * 255 / 100);
}

/**
 * 设计稿的公共阴影：box-shadow: 0 4px 12px rgba(110,100,200,.16)
 */
static void set_shadow(lv_style_t *st) {
  lv_style_set_shadow_color(st, lv_color_hex(MDA_SHADOW));
  lv_style_set_shadow_opa(st, opa(16));
  lv_style_set_shadow_width(st, 12);
  lv_style_set_shadow_offset_y(st, 4);
}

/**
 * 设计稿的公共白边：border: 1px solid rgba(255,255,255,.95)
 */
static void set_white_border(lv_style_t *st) {
  lv_style_set_border_color(st, lv_color_white());
  lv_style_set_border_width(st, 1);
  lv_style_set_border_opa(st, opa(95));
}

/**
 * 两色渐变：上面浅色 → 下面深色（设计稿是 160° 斜向，在小方块上和竖向看不出区别）
 */
static void init_grad(lv_style_t *st, uint32_t top, uint32_t bottom) {
  lv_style_init(st);
  lv_style_set_bg_opa(st, LV_OPA_COVER);
  lv_style_set_bg_color(st, lv_color_hex(top));
  lv_style_set_bg_grad_color(st, lv_color_hex(bottom));
  lv_style_set_bg_grad_dir(st, LV_GRAD_DIR_VER);
}

static void init_text(lv_style_t *st, const lv_font_t *font, uint32_t color) {
  lv_style_init(st);
  lv_style_set_text_font(st, font);
  lv_style_set_text_color(st, lv_color_hex(color));
}

/**
 * 初始化所有公共样式。必须在 lv_init() 之后、创建任何页面之前调用一次。
 */
void theme_init() {
  // 卡片 = 设计稿 .cardx
  lv_style_init(&st_card);
  lv_style_set_bg_color(&st_card, lv_color_white());
  lv_style_set_bg_opa(&st_card, opa(72));
  lv_style_set_radius(&st_card, 14);
  set_white_border(&st_card);
  set_shadow(&st_card);
  lv_style_set_pad_all(&st_card, 0);

  // 更实的卡片 = 设计稿 --card-strong（气泡、返回按钮），不要白边
  lv_style_init(&st_card_strong);
  lv_style_set_bg_color(&st_card_strong, lv_color_white());
  lv_style_set_bg_opa(&st_card_strong, opa(90));
  lv_style_set_radius(&st_card_strong, 12);
  lv_style_set_border_width(&st_card_strong, 0);
  set_shadow(&st_card_strong);
  lv_style_set_pad_all(&st_card_strong, 0);

  // 首页入口块 = 设计稿 .tile；底色（TILE_PINK 等）创建时用 lv_obj_set_style_bg_color 单独设
  lv_style_init(&st_tile);
  lv_style_set_bg_opa(&st_tile, LV_OPA_COVER);
  lv_style_set_radius(&st_tile, 14);
  set_white_border(&st_tile);
  set_shadow(&st_tile);
  lv_style_set_pad_all(&st_tile, 0);
  lv_style_set_text_font(&st_tile, &font_cn_11);
  lv_style_set_text_color(&st_tile, lv_color_hex(MDA_INK));

  // 图标方块 = 设计稿 .tile .ic；颜色由叠加的 st_grad_xxx 决定
  lv_style_init(&st_icon_box);
  lv_style_set_radius(&st_icon_box, 9);
  lv_style_set_border_width(&st_icon_box, 0);
  lv_style_set_pad_all(&st_icon_box, 0);
  lv_style_set_text_color(&st_icon_box, lv_color_white());

  init_grad(&st_grad_pink, MDA_PINK_L, MDA_PINK);
  init_grad(&st_grad_blue, MDA_BLUE_L, MDA_BLUE);
  init_grad(&st_grad_green, MDA_GREEN_L, MDA_GREEN);
  init_grad(&st_grad_violet, MDA_VIOLET_L, MDA_VIOLET);

  // 文字
  init_text(&st_text, &font_cn_11, MDA_INK);
  init_text(&st_text2, &font_cn_11, MDA_INK2);
  init_text(&st_title, &font_cn_18, MDA_INK);
}
