#ifndef MDA_ROBOTS_H
#define MDA_ROBOTS_H

#include <lvgl.h>
#include "ui/mda_theme.h"
#include "assets/images/mda_images.h"

typedef struct {
    const char *name;              // 名字（字符串常量是只读的，要用 const char *
    const lv_image_dsc_t *image_l;   // 首页大图
    const lv_image_dsc_t *image_m;   // setting 页面
    const lv_image_dsc_t *image_g;   // grid list 页面
}mda_robot_image_info_t;

// 更新，将最新的配置写入sd，返回上一次使用的
uint32_t mda_robot_update(mda_robot_image_info_t *robot);
uint32_t mda_robot_size();
const mda_robot_image_info_t *mda_get_robot(int index);

// 读取sd配置，开机只读取一次，如果没有则初始化第一个
void mda_robot_load();

#endif
