/**
 * 仿真用的状态栏占位：真正的 mda_statusbar.cpp 直接读 M5.Power 和 WiFi，电脑上编译不了，
 * env:sim 里把它排除了，这里给一个空实现让 ui_init() 能链接通过。
 * 之后把状态栏的硬件读取抽成接口（第 3 步）后，这个文件就可以删掉。
 */
#ifdef MDA_SIM

#include "ui/widgets/mda_statusbar.h"

void mda_statusbar_create() {
}

#endif
