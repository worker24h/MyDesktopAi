#ifndef MDA_ROBOT_CONFIG_H
#define MDA_ROBOT_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// 保存机器人选中的 ID（真机写入 SD 卡，仿真在本地模拟）
bool mda_robot_config_save(int32_t active_id);

// 读取机器人选中的 ID，读取成功返回 true
bool mda_robot_config_load(int32_t &active_id);

#endif
