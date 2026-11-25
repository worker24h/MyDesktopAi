#include <stdio.h>
#include <mda_logger.h>
#include "mda_menubar.h"
#include "../mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "mda_statusbar.h"

static mda_pages_t *mda_pages_ptr;
static lv_obj_t *icon_actived;

/**
 * 创建一个菜单图标。位置由父容器的 flex 布局决定，这里不用 lv_obj_align
 */
static lv_obj_t *mda_menubar_icon_create(lv_obj_t *parent, const char *icon) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE); // label默认不支持click事件
    lv_obj_set_ext_click_area(label, 8); //图标太小，扩大click区域
    lv_label_set_text(label, icon); // 设置icon
    // 纯图标字体：行高等于字号，没有中文字体基线下方的空白，flex 居中后图标就在正中间
    lv_obj_set_style_text_font(label, &font_icon_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(MDA_COLOR_NAV_OFF), 0);
    return label;
}

void mda_switch_page(mda_page_t *active_page) {
    lv_obj_t *icon = active_page->icon;
    lv_obj_set_style_text_color(icon_actived, lv_color_hex(MDA_COLOR_NAV_OFF), 0);    // 上一个恢复默认颜色
    lv_obj_set_style_text_color(icon, lv_color_hex(MDA_COLOR_PINK), 0);  // 当前页高亮

    //动画效果切换页面
    lv_screen_load_anim_t anim = (lv_obj_get_index(icon) > lv_obj_get_index(icon_actived))
                             ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                             : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_screen_load_anim(active_page->page, anim, 300, 0, false);
    icon_actived = icon;
}

static void mda_menubar_common_actived(lv_event_t *e) {
    lv_obj_t *icon = lv_event_get_current_target_obj(e);
    if (icon == icon_actived) {
        return;    // 就在当前页，忽略
    }
    if (mda_pages_ptr->home.icon == icon_actived) {
        mda_pages_ptr->last_page = &(mda_pages_ptr->home);
        logger.info("statusbar", "1 mda_pages_ptr->last_page=0x%p", mda_pages_ptr->last_page);
    } else if (mda_pages_ptr->smile.icon == icon_actived) {
        mda_pages_ptr->last_page = &(mda_pages_ptr->smile); 
        logger.info("statusbar", "2 mda_pages_ptr->last_page=0x%p", mda_pages_ptr->last_page);
    } else if (mda_pages_ptr->settings.icon == icon_actived) {
        mda_pages_ptr->last_page = &(mda_pages_ptr->settings);
        logger.info("statusbar", "3 mda_pages_ptr->last_page=0x%p", mda_pages_ptr->last_page);
    }
    mda_page_t *active_page = (mda_page_t *)lv_event_get_user_data(e);
    mda_switch_page(active_page);
    //lv_obj_remove_flag(pages->menu_bar, LV_OBJ_FLAG_HIDDEN);
    //将return btn设置隐藏，时间设置显示
    //mda_statusbar_notify(0);
}

// chat页面单独处理
static void mda_menubar_chat_icon_actived(lv_event_t *e) {
    lv_obj_t *icon = lv_event_get_current_target_obj(e);
    if (icon == icon_actived) {
        return;    // 点的就是当前页
    }
    if (mda_pages_ptr->home.icon == icon_actived) {
        mda_pages_ptr->last_page = &(mda_pages_ptr->home);
    } else if (mda_pages_ptr->smile.icon == icon_actived) {
        mda_pages_ptr->last_page = &(mda_pages_ptr->smile);
    } else if (mda_pages_ptr->settings.icon == icon_actived) {
        mda_pages_ptr->last_page = &(mda_pages_ptr->settings);
    }
    /**
     * 需要将menu_bar隐藏，换成声音btn
     * 将return btn设置显示，时间设置隐藏
     */ 
    lv_obj_t *parent = lv_obj_get_parent(icon); // parent=>menu_bar 
    lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
    mda_statusbar_set_mode(MDA_STATUSBAR_SUBPAGE, "AI助手");

    // 切换页面
    mda_page_t *active_page = (mda_page_t *)lv_event_get_user_data(e);
    mda_switch_page(active_page);
}


lv_obj_t *mda_menubar_create(mda_pages_t *pages) {
    mda_pages_ptr = pages;
    // 放在 top 层：盖在所有页面上面，切页面不用重建；放 bottom 层会被页面挡住，点击也收不到
    lv_obj_t *menubar = lv_obj_create(lv_layer_top());
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

    lv_obj_t *icon_home = mda_menubar_icon_create(menubar, ICON_HOME);
    lv_obj_set_style_text_color(icon_home, lv_color_hex(MDA_COLOR_PINK), 0);  // 当前页高亮
    lv_obj_add_event_cb(icon_home, mda_menubar_common_actived, LV_EVENT_CLICKED, &(mda_pages_ptr->home));
    mda_pages_ptr->home.icon = icon_home;
    icon_actived = icon_home;

    lv_obj_t *icon_smile = mda_menubar_icon_create(menubar, ICON_SMILE);
    lv_obj_add_event_cb(icon_smile, mda_menubar_common_actived, LV_EVENT_CLICKED, &(mda_pages_ptr->smile));
    mda_pages_ptr->smile.icon = icon_smile;

    lv_obj_t *icon_chat = mda_menubar_icon_create(menubar, ICON_MIC);
    lv_obj_add_event_cb(icon_chat, mda_menubar_chat_icon_actived, LV_EVENT_CLICKED, &(mda_pages_ptr->chat));
    mda_pages_ptr->chat.icon = icon_chat;

    lv_obj_t *icon_settings = mda_menubar_icon_create(menubar, ICON_SETTINGS);
    lv_obj_add_event_cb(icon_settings, mda_menubar_common_actived, LV_EVENT_CLICKED, &(mda_pages_ptr->settings));
    mda_pages_ptr->settings.icon = icon_settings;

    return menubar;
}



