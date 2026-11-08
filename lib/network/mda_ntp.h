#ifndef MDA_NTP_H
#define MDA_NTP_H

#include <stddef.h>

// 时间管理：CoreS3 板载 RTC（BM8563）+ NTP 对时。
//   - 开机：读 RTC 设置系统时间（RTC 里保存的是 UTC 时间）；
//   - STA 连上路由器后：向 NTP 服务器对时，成功后把准确时间写回 RTC，下次开机即使没网时间也是对的；
//   - 时区：中国标准时间 UTC+8。
// 系统时间有效后，localtime()、日志时间戳等都会显示真实时间。

// 初始化：RTC → 系统时间，设置时区。需要在 M5.begin() 之后调用。
void mda_ntp_initialize();

// 在 loop() 中反复调用：STA 连上时触发对时，对时成功后写回 RTC。
void mda_ntp_loop();

// 系统时间是否有效（来自 RTC 或 NTP，年份 >= 2024）
bool mda_time_valid();

// 把当前本地时间格式化为 "17:30:05"。时间无效时返回 false。
bool mda_time_format(char *buf, size_t len);

// 把当前本地时间格式化为 "2026-11-08 17:30:05"。时间无效时返回 false。
bool mda_datetime_format(char *buf, size_t len);

#endif
