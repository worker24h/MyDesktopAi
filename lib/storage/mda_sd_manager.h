#ifndef MDA_SD_MANAGER_H
#define MDA_SD_MANAGER_H

// SD 卡上的统一数据目录
#define MDA_DATA_DIR   "/data"
#define MDA_CONFIG_DIR "/data/config"
#define MDA_LOG_DIR    "/data/logs"

void sd_setup();

// 挂载 SD 卡（CS=4，与屏幕共用 SPI 总线）。已挂载时直接返回 true。
bool sd_mount();

// SD 卡当前是否已挂载
bool sd_is_mounted();

// 确保 /data、/data/config、/data/logs 目录存在，不存在就创建。全部就绪返回 true。
bool sd_init_dirs();

// 把 SD 卡格式化为 FAT32（MBR 分区表，和电脑上格式化的结果一致），完成后自动重新挂载。
// ⚠️ 会清空卡上所有数据，且无法恢复。调用前请确保没有打开的文件。
// 成功返回 true。卡太小不适合 FAT32 时自动改用 FAT16/FAT12。
bool sd_format();

#endif

