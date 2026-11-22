
#include <string.h>
#include <lvgl.h>
#ifndef MDA_SIM
// 硬件相关：电脑仿真（env:sim）里没有 Arduino / SD 卡，不参与编译
#include <mda_sd_manager.h>
#include <mda_logger.h>
#include <SD.h>
#include <ArduinoJson.h>
#endif
#include "config/mda_config.h"
#include "ui/mda_theme.h"
#include "mda_robots.h"

#define ROBOT_CONFIG_FILE "/data/config/robot.json"


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

lv_subject_t *mda_robot_get_subject() {
    return &subject_robot_active_id;
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

static bool robot_config_save() {
#ifndef MDA_SIM
    if (!sd_is_mounted()) {
        return false;
    }
    if (!sd_init_dirs()) {
        return false;
    }

    JsonDocument doc;
    doc["robot_active_id"] = robot_active_id;

    // 先完整写入临时文件
    File file = SD.open(ROBOT_CONFIG_FILE, FILE_WRITE);
    if (!file) {
        logger.warn("robot", "create robot config file failed.");
        return false;
    }
    size_t written = serializeJsonPretty(doc, file);
    file.close();
    if (written == 0) {
        logger.warn("robot", "write robot config file failed.");
        SD.remove(ROBOT_CONFIG_FILE);
        return false;
    }
    return true;
#else
    // 仿真里没有 SD 卡，假装保存成功
    return true;
#endif
}


// 更新，将最新的配置写入sd，返回上一次使用的
int32_t mda_robot_update(mda_robot_image_info_t *robot) {
    int32_t last_actived = robot_active_id;
    for (int i = 0; i < MDA_ARRAY_LEN(face_images); i++) {
        mda_robot_image_info_t face = face_images[i];
        if (strcmp(face.name, robot->name) == 0) {
            robot_active_id = i;
            robot_config_save();
            //notify subject
            lv_subject_set_int(&subject_robot_active_id, robot_active_id);
            break;
        }
    }
    return last_actived;
}

// 读取sd配置，开机只读取一次，如果没有则初始化第一个
static void mda_robot_load() {
#ifndef MDA_SIM
    if(!sd_is_mounted()) {
        return;
    }
    if (!SD.exists(ROBOT_CONFIG_FILE)) {
        logger.info("robot", "robot config file not exist.");
        return ;
    }
    File file = SD.open(ROBOT_CONFIG_FILE, "r");
    if (!file) {
        logger.warn("robot", "open robot config file failed.");
        return ;
    }
    static JsonDocument robot;
    DeserializationError err = deserializeJson(robot, file);
    file.close();
    if (err) {
        logger.warn("robot", "deserialize json failed");
        return ;
    }
    robot_active_id = robot["robot_active_id"] | 0;
#endif
    // 仿真里不读配置，robot_active_id 保持默认值 0
}


void mda_robot_initalize() {
    mda_robot_load();

    //初始化
    lv_subject_init_int(&subject_robot_active_id, robot_active_id);
}