#ifndef MDA_EXPR_H
#define MDA_EXPR_H

#include <lvgl.h>

// 机器人表情：编号 + 名字 + 各尺寸图片集中在这里，各页面只认 mda_expr_t。
// 顺序和名字取自设计稿 docs/design/cores3-ui.html 的 EXPR 对象。
typedef enum {
    MDA_EXPR_HAPPY,      // 开心
    MDA_EXPR_LAUGH,      // 大笑
    MDA_EXPR_SURPRISED,  // 惊讶
    MDA_EXPR_THINK,      // 思考
    MDA_EXPR_LISTEN,     // 倾听
    MDA_EXPR_SING,       // 唱歌
    MDA_EXPR_SLEEP,      // 睡觉
    MDA_EXPR_SAD,        // 难过
    MDA_EXPR_EXCITED,    // 兴奋
    MDA_EXPR_PLAYFUL,    // 调皮
    MDA_EXPR_CONFUSED,   // 疑惑
    MDA_EXPR_TALK,       // 说话
    MDA_EXPR_COUNT,      // 总数，用来写循环和数组大小，不是一种表情
} mda_expr_t;

typedef struct {
    const char *key;               // 保存到配置文件用的英文名，如 "happy"（存字符串不存编号，插入新表情也不会错位）
    const char *name;              // 显示用的中文名，如 "开心"
    const lv_image_dsc_t *img_s;   // 44px：表情网格
    const lv_image_dsc_t *img_m;   // 80px：设置页头像
    const lv_image_dsc_t *img_l;   // 120px：首页
} mda_expr_info_t;

// 取表情信息。越界时返回「开心」，不会返回空指针
const mda_expr_info_t *mda_expr_info(mda_expr_t expr);

// "happy" → MDA_EXPR_HAPPY；为空或找不到时返回 MDA_EXPR_HAPPY
mda_expr_t mda_expr_from_key(const char *key);

// 当前表情。现在只保存在内存里，接入 mda_ui_config 后改为读写配置（接口不变）
mda_expr_t mda_expr_current();
void mda_expr_set_current(mda_expr_t expr);

#endif
