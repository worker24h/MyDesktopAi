
#include <lvgl.h>
#include "mda_toast.h"


static lv_obj_t *s_toast_obj = nullptr;
static lv_timer_t *s_toast_timer = nullptr;

static void toast_opa_anim_cb(void *var, int32_t v) {
    lv_obj_t *obj = (lv_obj_t *)var;
    if (lv_obj_is_valid(obj)) {
        lv_obj_set_style_opa(obj, (lv_opa_t)v, 0);
    }
}

static void toast_delete_ready_cb(lv_anim_t *a) {
    LV_UNUSED(a);
    if (s_toast_obj && lv_obj_is_valid(s_toast_obj)) {
        lv_obj_delete(s_toast_obj);
    }
    s_toast_obj = nullptr;
}

static void toast_hide_timer_cb(lv_timer_t *t) {
    LV_UNUSED(t);
    s_toast_timer = nullptr;

    if (!s_toast_obj || !lv_obj_is_valid(s_toast_obj)) {
        s_toast_obj = nullptr;
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_toast_obj);
    lv_anim_set_values(&a, lv_obj_get_style_opa(s_toast_obj, 0), LV_OPA_TRANSP);
    lv_anim_set_duration(&a, 220);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&a, toast_opa_anim_cb);
    lv_anim_set_completed_cb(&a, toast_delete_ready_cb);
    lv_anim_start(&a);
}

/**
 * 弹出 Toast 提示
 * @param text      提示文案（UTF-8）
 * @param font      字体指针（如 &font_cn_12b）
 * @param duration  停留时长（毫秒），例如 1500
 */
void mda_toast_show(const char *text, const lv_font_t *font, uint32_t duration_ms) {
    // 若已有定时器或旧 Toast，先清理并复用
    if (s_toast_timer) {
        lv_timer_delete(s_toast_timer);
        s_toast_timer = nullptr;
    }
    if (s_toast_obj && lv_obj_is_valid(s_toast_obj)) {
        lv_anim_delete(s_toast_obj, toast_opa_anim_cb);
        lv_obj_delete(s_toast_obj);
        s_toast_obj = nullptr;
    }

    // 挂载到顶层 layer_top，跨页面切换也能正常显示
    lv_obj_t *toast = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(toast);
    lv_obj_set_size(toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(toast, 260, 0); // 适配 320px 宽屏，超长自动换行
    lv_obj_set_style_pad_hor(toast, 14, 0);
    lv_obj_set_style_pad_ver(toast, 8, 0);
    lv_obj_set_style_radius(toast, 12, 0);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0x1E293B), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_90, 0);
    lv_obj_set_style_opa(toast, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
    // 关键：不拦截触摸事件，避免挡住底层按钮点击
    lv_obj_remove_flag(toast, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *label = lv_label_create(toast);
    if (font) {
        lv_obj_set_style_text_font(label, font, 0);
    }
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, text ? text : "");
    lv_obj_center(label);

    s_toast_obj = toast;

    // 淡入动画
    lv_anim_t in_anim;
    lv_anim_init(&in_anim);
    lv_anim_set_var(&in_anim, toast);
    lv_anim_set_values(&in_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&in_anim, 160);
    lv_anim_set_path_cb(&in_anim, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&in_anim, toast_opa_anim_cb);
    lv_anim_start(&in_anim);

    // 定时触发淡出
    s_toast_timer = lv_timer_create(toast_hide_timer_cb, duration_ms, nullptr);
    lv_timer_set_repeat_count(s_toast_timer, 1);
}