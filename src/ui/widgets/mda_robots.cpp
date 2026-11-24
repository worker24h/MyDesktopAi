
#include <string.h>
#include <lvgl.h>
#include "config/mda_robot_config.h"
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "mda_robots.h"



static const mda_robot_image_info_t face_images[] = {
    {
        .name    = "开心",
        .image_l = &robot_happy_L,
        .image_m = &robot_happy_M,
        .image_g = &robot_happy_G,
    },
    {
        .name    = "大笑",
        .image_l = &robot_laugh_L,
        .image_m = &robot_laugh_M,
        .image_g = &robot_laugh_G,
    },
    {
        .name    = "惊讶",
        .image_l = &robot_surprised_L,
        .image_m = &robot_surprised_M,
        .image_g = &robot_surprised_G,
    },
    {
        .name    = "思考",
        .image_l = &robot_think_L,
        .image_m = &robot_think_M,
        .image_g = &robot_think_G,
    },
    {
        .name    = "倾听",
        .image_l = &robot_listen_L,
        .image_m = &robot_listen_M,
        .image_g = &robot_listen_G,
    },
    {
        .name    = "唱歌",
        .image_l = &robot_sing_L,
        .image_m = &robot_sing_M,
        .image_g = &robot_sing_G,
    },
    {
        .name    = "睡觉",
        .image_l = &robot_sleep_L,
        .image_m = &robot_sleep_M,
        .image_g = &robot_sleep_G,
    },
    {
        .name    = "难过",
        .image_l = &robot_sad_L,
        .image_m = &robot_sad_M,
        .image_g = &robot_sad_G,
    },
    {
        .name    = "兴奋",
        .image_l = &robot_excited_L,
        .image_m = &robot_excited_M,
        .image_g = &robot_excited_G,
    },
    {
        .name    = "调皮",
        .image_l = &robot_playful_L,
        .image_m = &robot_playful_M,
        .image_g = &robot_playful_G,
    },
    {
        .name    = "疑惑",
        .image_l = &robot_confused_L,
        .image_m = &robot_confused_M,
        .image_g = &robot_confused_G,
    },
    {
        .name    = "说话",
        .image_l = &robot_talk_L,
        .image_m = &robot_talk_M,
        .image_g = &robot_talk_G,
    },
};


static int32_t robot_active_id = 0; //默认激活的id
static lv_subject_t subject_robot_active_id; //订阅

int32_t mda_robot_actived_index() {
    return robot_active_id;
}

void mda_robot_subscriber(lv_observer_cb_t cb, void *user_data) {
    lv_subject_add_observer(&subject_robot_active_id, cb, user_data);
}

int32_t mda_robot_size() {
    return MDA_ARRAY_LEN(face_images);
}

const mda_robot_image_info_t *mda_get_robot(int index) {
    return &face_images[index];
}

const mda_robot_image_info_t *mda_get_actived_robot() {
    return &face_images[robot_active_id];
}

// 更新，将最新的配置写入存储，返回上一次使用的
int32_t mda_robot_update(mda_robot_image_info_t *robot) {
    int32_t last_actived = robot_active_id;
    for (int i = 0; i < MDA_ARRAY_LEN(face_images); i++) {
        mda_robot_image_info_t face = face_images[i];
        if (strcmp(face.name, robot->name) == 0) {
            robot_active_id = i;
            mda_robot_config_save(robot_active_id);
            //notify subject
            lv_subject_set_int(&subject_robot_active_id, robot_active_id);
            break;
        }
    }
    return last_actived;
}

void mda_robot_initalize() {
    mda_robot_config_load(robot_active_id);

    //初始化
    lv_subject_init_int(&subject_robot_active_id, robot_active_id);
}