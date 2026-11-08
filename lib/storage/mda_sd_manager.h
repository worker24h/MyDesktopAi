#ifndef MDA_SD_MANAGER_H
#define MDA_SD_MANAGER_H

void sd_setup();

// 把 SD 卡格式化为 FAT32（MBR 分区表，和电脑上格式化的结果一致），完成后自动重新挂载。
// ⚠️ 会清空卡上所有数据，且无法恢复。调用前请确保没有打开的文件。
// 成功返回 true。卡太小不适合 FAT32 时自动改用 FAT16/FAT12。
bool sd_format();

#endif

