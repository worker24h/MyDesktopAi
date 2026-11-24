#include "mda_robot_config.h"
#include <mda_sd_manager.h>
#include <mda_logger.h>
#include <SD.h>
#include <ArduinoJson.h>

#define ROBOT_CONFIG_FILE "/data/config/robot.json"

bool mda_robot_config_save(int32_t active_id) {
    if (!sd_is_mounted() || !sd_init_dirs()) {
        return false;
    }

    JsonDocument doc;
    doc["robot_active_id"] = active_id;

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
}

bool mda_robot_config_load(int32_t &active_id) {
    if (!sd_is_mounted()) {
        return false;
    }
    if (!SD.exists(ROBOT_CONFIG_FILE)) {
        logger.info("robot", "robot config file not exist.");
        return false;
    }
    File file = SD.open(ROBOT_CONFIG_FILE, "r");
    if (!file) {
        logger.warn("robot", "open robot config file failed.");
        return false;
    }
    JsonDocument robot;
    DeserializationError err = deserializeJson(robot, file);
    file.close();
    if (err) {
        logger.warn("robot", "deserialize json failed");
        return false;
    }
    active_id = robot["robot_active_id"] | 0;
    return true;
}
