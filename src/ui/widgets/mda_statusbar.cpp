#include <string.h>
#include <lvgl.h>
#include "mda_battery.h"
#include "mda_toast.h"
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "assets/fonts/mda_fonts.h"
#include "mda_ntp.h"
#include "mda_statusbar.h"
#include "mda_menubar.h"
#include "mda_network.h"
#include "mda_logger.h"

#define MAX_STATUSBAR_HEIGHT 22
#define TIME_WIFI_REFRESH_MS 1000   // 时间&wifi刷新时间
#define BATTERY_REFRESH_MS 2000     // 电量刷新间隔

static lv_obj_t *time_label = nullptr;      // 根页面：时间
static lv_obj_t *return_btn = nullptr;      // 二级页面：返回按钮
static lv_obj_t *s_title_label = nullptr;   // 二级页面：页面标题
static const mda_pages_t *s_pages_ptr = nullptr;

// lv_timer 回调：由 loop() 里的 lv_timer_handler() 定时调用
static void mda_wifi_event_cb(lv_timer_t *timer) {
    // LVGL v9 里 lv_timer_t 是私有结构体，不能直接访问成员，要用访问函数
    lv_obj_t *wifi_label = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (wifi_sta_get_status().state == Connected) {
        lv_label_set_text(wifi_label, ICON_WIFI);
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(MDA_COLOR_INK), 0);
    } else {
        lv_label_set_text(wifi_label, ICON_WIFI_OFF);
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(MDA_COLOR_RED), 0);    // 断网显示红色
    }
    return;
}

// 使用icon svg
static void mda_wifi_create(lv_obj_t *parent) {
    lv_obj_t *wifi_label = lv_label_create(parent);
    lv_obj_add_style(wifi_label, &st_text, 0);                             // 默认颜色用 MDA_COLOR_INK
    lv_obj_set_style_text_font(wifi_label, &font_icon_20, 0);             // 图标字体覆盖 st_text 里的字体
    lv_obj_align(wifi_label, LV_ALIGN_RIGHT_MID, -40, 0);
    if (wifi_sta_get_status().state == Connected) {
        lv_label_set_text(wifi_label, ICON_WIFI);
    } else {
        lv_label_set_text(wifi_label, ICON_WIFI_OFF);
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(MDA_COLOR_RED), 0);    // 断网显示红色
    }
    lv_timer_create(mda_wifi_event_cb, TIME_WIFI_REFRESH_MS, wifi_label);
}

// 根据当前电源状态选图标和颜色
static void mda_battery_update(lv_obj_t *battery_label) {
    lv_obj_set_style_text_color(battery_label, lv_color_hex(MDA_COLOR_INK), 0); // 默认颜色
    if (mda_is_external_battery()) { // 说明是外接电源供电, 且不是输出
        lv_label_set_text(battery_label, ICON_BATTERY_FULL);
        return;
    }
    if (mda_is_charging()) {
        lv_label_set_text(battery_label, ICON_BATTERY_CHARGIN);
    } else {
        int battery_level = mda_battery_level();
        if (battery_level >= 90) {
            lv_label_set_text(battery_label, ICON_BATTERY_FULL);
        } else if (battery_level >= 30) {
            lv_label_set_text(battery_label, ICON_BATTERY_MEDIUM);
        } else {
            lv_label_set_text(battery_label, ICON_BATTERY_LOW);
            lv_obj_set_style_text_color(battery_label, lv_color_hex(MDA_COLOR_RED), 0);    // 电量低显示红色
        }
    }
}

// lv_timer 回调：由 loop() 里的 lv_timer_handler() 定时调用
static void mda_battery_event_cb(lv_timer_t *timer) {
    lv_obj_t *battery_label = (lv_obj_t *)lv_timer_get_user_data(timer);
    mda_battery_update(battery_label);
    return;
}

static void mda_battery_create(lv_obj_t *parent) {
    lv_obj_t *s_battery_label = lv_label_create(parent);
    lv_obj_add_style(s_battery_label, &st_text, 0);                                // 默认颜色用 MDA_COLOR_INK
    lv_obj_set_style_text_font(s_battery_label, &font_icon_24, 0);                 // 电池图标偏扁，用 18px 才看得清
    lv_obj_align(s_battery_label, LV_ALIGN_RIGHT_MID, -10, 0);

    mda_battery_update(s_battery_label);                                            // 先显示一次，不用等第一个周期
    lv_timer_create(mda_battery_event_cb, BATTERY_REFRESH_MS, s_battery_label);
}

// lv_timer 回调：由 loop() 里的 lv_timer_handler() 定时调用
static void mda_timer_event_cb(lv_timer_t *timer) {
    lv_obj_t *label = (lv_obj_t *)lv_timer_get_user_data(timer);
    char buf[8] = {0};
    if (!mda_date_format_custom("%H:%M", buf, sizeof(buf))) {
        strncpy(buf, "--:--", 6);
    }
    lv_label_set_text(label, buf);
    return;
}

static lv_obj_t *mda_time_create(lv_obj_t *parent) {
    char buf[8] = {0};
    if (!mda_date_format_custom("%H:%M", buf, sizeof(buf))) {
        strncpy(buf, "--:--", 6);
    }
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_add_style(label, &st_text, 0);
    lv_obj_set_style_text_font(label, &font_num_13b, 0);
    lv_label_set_text(label, buf);
    lv_timer_create(mda_timer_event_cb, TIME_WIFI_REFRESH_MS, label);
    return label;
}

// 点击左上角返回按钮
static void return_btn_event_cb(lv_event_t *e) {
    LV_UNUSED(e);
    if (s_pages_ptr) {
        // 1. 恢复底部菜单栏
        if (s_pages_ptr->menu_bar) {
            lv_obj_remove_flag(s_pages_ptr->menu_bar, LV_OBJ_FLAG_HIDDEN);
        }
        // 2. 切回上一次页面
        if (s_pages_ptr->last_page) {
            mda_switch_page(s_pages_ptr->last_page);
        }
    }
    // 3. 状态栏切回根模式：显时间，隐返回区
    mda_statusbar_set_mode(MDA_STATUSBAR_ROOT);
}

// 返回按钮容器（return_btn + title_label 横向 flex）
static lv_obj_t *mda_return_area_create(lv_obj_t *parent) {
    // 外层 flex 容器
    lv_obj_t *flex = lv_obj_create(parent);
    lv_obj_remove_style_all(flex);
    lv_obj_set_size(flex, LV_SIZE_CONTENT, MAX_STATUSBAR_HEIGHT);
    lv_obj_set_flex_flow(flex, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(flex, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(flex, 6, 0);
    lv_obj_add_flag(flex, LV_OBJ_FLAG_HIDDEN); // 默认隐藏

    // 返回按钮：28×18，白色胶囊，圆角 9px
    lv_obj_t *btn = lv_button_create(flex);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 28, 18);
    lv_obj_set_style_radius(btn, 9, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(MDA_COLOR_WHITE), 0);
    lv_obj_set_ext_click_area(btn, 10);
    lv_obj_add_event_cb(btn, return_btn_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *arrow = lv_label_create(btn);
    lv_obj_add_style(arrow, &st_text, 0);
    lv_obj_set_style_text_font(arrow, &font_icon_20, 0);
    lv_label_set_text(arrow, ICON_LEFT);
    lv_obj_center(arrow);

    // 页面标题（唯一的 title_label，set_mode 时更新文字）
    s_title_label = lv_label_create(flex);
    lv_obj_add_style(s_title_label, &st_text, 0);
    lv_obj_set_style_text_font(s_title_label, &font_cn_11b, 0);
    lv_label_set_text(s_title_label, "");

    return flex;
}

// 左上角复合区域：时间 / 返回区 互斥叠放（都附在同一父容器，各自独立显隐）
static void mda_left_area_create(lv_obj_t *parent) {
    time_label = mda_time_create(parent);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 10, 0);

    return_btn = mda_return_area_create(parent);
    lv_obj_align(return_btn, LV_ALIGN_LEFT_MID, 10, 0);
}

void mda_statusbar_set_mode(mda_statusbar_mode_t mode, const char *title) {
    if (mode == MDA_STATUSBAR_SUBPAGE) {
        if (title && s_title_label) {
            lv_label_set_text(s_title_label, title);
        }
        lv_obj_add_flag(time_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(time_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
    }
}


lv_obj_t * mda_statusbar_create(const mda_pages_t *pages) {
    s_pages_ptr = pages;

    lv_obj_t *status_bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(status_bar);    // 透明容器，不要默认的白底和边框
    lv_obj_set_size(status_bar, MDA_SCREEN_WIDTH, MAX_STATUSBAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE); // 不可拖动
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);  // 不可点击

    // 左上角复合区域：时间 / 返回区 互斥叠放
    mda_left_area_create(status_bar);
    mda_wifi_create(status_bar);
    mda_battery_create(status_bar);

    return status_bar;
}
