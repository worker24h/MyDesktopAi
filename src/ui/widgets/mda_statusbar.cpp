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

static lv_subject_t subject_status_bar; // 订阅对象
static lv_obj_t *time_label = nullptr;
static lv_obj_t *return_btn_flex = nullptr;
static lv_obj_t *s_title_label = nullptr;
static mda_pages_t *s_pages_ptr = nullptr;

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
    lv_obj_add_style(label, &st_text, 0);                                        // 颜色用 MDA_COLOR_INK
    lv_obj_set_style_text_font(label, &font_num_13b, 0);                         // 粗体数字
    lv_label_set_text(label, buf);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 0);
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
        // 2. 切回首页（向右滑动动画）
        if (s_pages_ptr->home) {
            lv_screen_load_anim(s_pages_ptr->home, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        }
        // 3. 恢复底部菜单栏 Home 图标高亮
        mda_menubar_set_active_home();
    }
    // 4. 状态栏恢复时间显示，隐藏返回按钮
    mda_statusbar_notify(0);
}

static lv_obj_t *mda_return_create(lv_obj_t *parent) {
    // 容器：横向排列，靠左垂直居中，左边距 10px 与时间对齐
    lv_obj_t *flex = lv_obj_create(parent);
    lv_obj_remove_style_all(flex);
    lv_obj_set_size(flex, LV_SIZE_CONTENT, MAX_STATUSBAR_HEIGHT);
    lv_obj_align(flex, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_flex_flow(flex, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(flex, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(flex, 6, 0);        // 按钮与文字间距 6px
    lv_obj_add_flag(flex, LV_OBJ_FLAG_HIDDEN);      // 默认不显示

    // 返回按钮：设计稿 28x18，白色胶囊圆角 9px
    lv_obj_t *btn = lv_button_create(flex);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 28, 18);
    lv_obj_set_style_radius(btn, 9, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(MDA_COLOR_WHITE), 0);
    lv_obj_set_ext_click_area(btn, 10);
    lv_obj_add_event_cb(btn, return_btn_event_cb, LV_EVENT_CLICKED, nullptr);

    // 返回箭头图标 <
    lv_obj_t *return_label = lv_label_create(btn);
    lv_obj_add_style(return_label, &st_text, 0);   // 颜色用 MDA_COLOR_INK
    lv_obj_set_style_text_font(return_label, &font_icon_20, 0);
    lv_label_set_text(return_label, ICON_LEFT);
    lv_obj_center(return_label);

    // 页面标题文字
    s_title_label = lv_label_create(flex);
    lv_obj_add_style(s_title_label, &st_text, 0);
    lv_obj_set_style_text_font(s_title_label, &font_cn_11b, 0);
    lv_label_set_text(s_title_label, "AI助手");

    return flex;
}

static void mda_statusbar_observer_cb(lv_observer_t * observer, lv_subject_t * subject) {
    LV_UNUSED(observer);
    int32_t return_show = lv_subject_get_int(subject);
    logger.info("statusbar", "observer: return_show=%d", return_show);
    if (return_show == 1) {
        lv_obj_add_flag(time_label, LV_OBJ_FLAG_HIDDEN);          // time设置隐藏
        lv_obj_remove_flag(return_btn_flex, LV_OBJ_FLAG_HIDDEN);  // return_btn设置显示
    } else {
        lv_obj_remove_flag(time_label, LV_OBJ_FLAG_HIDDEN);       // time设置显示
        lv_obj_add_flag(return_btn_flex, LV_OBJ_FLAG_HIDDEN);     // return_btn设置隐藏
    }
}

void mda_statusbar_notify(int32_t value, const char *title) {
    if (title && s_title_label) {
        lv_label_set_text(s_title_label, title);
    }
    lv_subject_set_int(&subject_status_bar, value);
}

void mda_statusbar_notify(int32_t value) {
    mda_statusbar_notify(value, nullptr);
}

lv_obj_t * mda_statusbar_create(mda_pages_t *pages) {
    s_pages_ptr = pages;

    lv_obj_t *status_bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(status_bar);    // 透明容器，不要默认的白底和边框
    lv_obj_set_size(status_bar, MDA_SCREEN_WIDTH, MAX_STATUSBAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE); // 不可拖动
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);  // 不可点击

    // 创建时间、Wi-Fi、和电量图标
    time_label = mda_time_create(status_bar);
    return_btn_flex = mda_return_create(status_bar);
    mda_wifi_create(status_bar);
    mda_battery_create(status_bar);

    // 设置订阅
    lv_subject_init_int(&subject_status_bar, 0);
    lv_subject_add_observer(&subject_status_bar, mda_statusbar_observer_cb, NULL);
    return status_bar;
}
