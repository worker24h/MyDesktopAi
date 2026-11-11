#ifndef MDA_THEME_H
#define MDA_THEME_H

#include <lvgl.h>

// 颜色：数值取自设计稿 docs/design/cores3-ui.html 的 CSS
constexpr uint32_t MDA_INK = 0x2A2F4A,  MDA_INK2 = 0x6B7194, MDA_LINE = 0xDADCEE;
constexpr uint32_t MDA_PINK = 0xFF5B97, MDA_PINK_L = 0xFF8FBD;
constexpr uint32_t MDA_BLUE = 0x3D8BFF, MDA_BLUE_L = 0x7CC0FF;
constexpr uint32_t MDA_GREEN = 0x26B773, MDA_GREEN_L = 0x6FE0A2, MDA_OK = 0x36C98A;
constexpr uint32_t MDA_VIOLET = 0x8B7CFF, MDA_VIOLET_L = 0xB7A9FF;
constexpr uint32_t MDA_RED = 0xFF4D6D;
constexpr uint32_t MDA_NAV_OFF = 0x6F7598;  // 导航栏未选中图标
constexpr uint32_t MDA_SHADOW = 0x6E64C8;   // 卡片阴影 rgba(110,100,200,.16)
// 入口块底色
constexpr uint32_t MDA_TILE_PINK = 0xFFD6E7, MDA_TILE_BLUE = 0xD9E8FF, MDA_TILE_GREEN = 0xD4F6E0, MDA_TILE_VIOLET = 0xE6E1FF;
// 聊天气泡
constexpr uint32_t MDA_BUBBLE_BOT = 0xDCF6E5;

// 公共样式：theme_init() 之后才能使用
extern lv_style_t st_card;         // 卡片：白色 72% 半透明 + 白边 + 阴影（设计稿 .cardx）
extern lv_style_t st_card_strong;  // 更实的卡片：白色 90%，用于气泡、返回按钮
extern lv_style_t st_tile;         // 首页入口块：圆角 + 白边 + 阴影，底色在创建时单独设置
extern lv_style_t st_icon_box;     // 图标方块：圆角 9 + 白色图标，再叠加一个 st_grad_xxx 决定颜色
extern lv_style_t st_grad_pink, st_grad_blue, st_grad_green, st_grad_violet;  // 两色竖向渐变
extern lv_style_t st_text;         // 正文：11px 深色
extern lv_style_t st_text2;        // 次要文字：11px 灰色
extern lv_style_t st_title;        // 标题 / 大数字：18px 深色

void theme_init();

#endif
