#include "config/mda_robot_config.h"
#include "mda_logger.h"

static int32_t s_sim_robot_active_id = 0;

bool mda_robot_config_save(int32_t active_id) {
    s_sim_robot_active_id = active_id;
    logger.info("sim", "Robot active ID saved: %d", active_id);
    return true;
}

bool mda_robot_config_load(int32_t &active_id) {
    active_id = s_sim_robot_active_id;
    logger.info("sim", "Robot active ID loaded: %d", active_id);
    return true;
}
