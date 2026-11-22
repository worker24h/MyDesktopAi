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

int32_t mda_robot_size();
int32_t mda_robot_actived_index();
lv_subject_t *mda_robot_get_subject();
void mda_robot_subscriber(lv_observer_cb_t cb, void *user_data);
// 更新，将最新的配置写入sd，返回上一次使用的
int32_t mda_robot_update(mda_robot_image_info_t *robot);
const mda_robot_image_info_t *mda_get_robot(int index);
const mda_robot_image_info_t *mda_get_actived_robot();

void mda_robot_initalize();

#endif
