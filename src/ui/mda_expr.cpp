#include <string.h>
#include "mda_expr.h"

// 每种表情声明 S / M / L 三个尺寸的图片（定义在 src/assets/images/robot_<表情>_<尺寸>.c）
#define MDA_EXPR_DECLARE(key) \
    LV_IMAGE_DECLARE(robot_##key##_S); \
    LV_IMAGE_DECLARE(robot_##key##_M); \
    LV_IMAGE_DECLARE(robot_##key##_L)

MDA_EXPR_DECLARE(happy);
MDA_EXPR_DECLARE(laugh);
MDA_EXPR_DECLARE(surprised);
MDA_EXPR_DECLARE(think);
MDA_EXPR_DECLARE(listen);
MDA_EXPR_DECLARE(sing);
MDA_EXPR_DECLARE(sleep);
MDA_EXPR_DECLARE(sad);
MDA_EXPR_DECLARE(excited);
MDA_EXPR_DECLARE(playful);
MDA_EXPR_DECLARE(confused);
MDA_EXPR_DECLARE(talk);

// 表里的一项：英文名、中文名、三个尺寸的图片
#define MDA_EXPR_ITEM(key, name) {#key, name, &robot_##key##_S, &robot_##key##_M, &robot_##key##_L}

// 顺序必须和 mda_expr_t 一致
static const mda_expr_info_t EXPR_TABLE[MDA_EXPR_COUNT] = {
    MDA_EXPR_ITEM(happy, "开心"),
    MDA_EXPR_ITEM(laugh, "大笑"),
    MDA_EXPR_ITEM(surprised, "惊讶"),
    MDA_EXPR_ITEM(think, "思考"),
    MDA_EXPR_ITEM(listen, "倾听"),
    MDA_EXPR_ITEM(sing, "唱歌"),
    MDA_EXPR_ITEM(sleep, "睡觉"),
    MDA_EXPR_ITEM(sad, "难过"),
    MDA_EXPR_ITEM(excited, "兴奋"),
    MDA_EXPR_ITEM(playful, "调皮"),
    MDA_EXPR_ITEM(confused, "疑惑"),
    MDA_EXPR_ITEM(talk, "说话"),
};

static mda_expr_t s_current = MDA_EXPR_HAPPY;

const mda_expr_info_t *mda_expr_info(mda_expr_t expr) {
    if (expr < 0 || expr >= MDA_EXPR_COUNT) {
        expr = MDA_EXPR_HAPPY;
    }
    return &EXPR_TABLE[expr];
}

mda_expr_t mda_expr_from_key(const char *key) {
    if (key == nullptr) {
        return MDA_EXPR_HAPPY;
    }
    for (int i = 0; i < MDA_EXPR_COUNT; i++) {
        if (strcmp(EXPR_TABLE[i].key, key) == 0) {
            return (mda_expr_t)i;
        }
    }
    return MDA_EXPR_HAPPY;
}

mda_expr_t mda_expr_current() {
    return s_current;
}

void mda_expr_set_current(mda_expr_t expr) {
    if (expr < 0 || expr >= MDA_EXPR_COUNT) {
        return;
    }
    s_current = expr;
}
