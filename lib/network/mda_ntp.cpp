#include "mda_ntp.h"

#include <M5Unified.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include "mda_logger.h"
#include "mda_network.h"

// POSIX 时区写法：CST-8 表示比 UTC 快 8 小时（符号与习惯相反）
#define NTP_TIMEZONE "CST-8"
#define NTP_SERVER1  "ntp.aliyun.com"
#define NTP_SERVER2  "0.pool.ntp.org"
#define NTP_SERVER3  "1.pool.ntp.org"

static const int VALID_MIN_YEAR = 2024;

// SNTP 回调在 lwIP 任务里执行，只置标志；写 RTC（I2C）放到 loop 里做，避免和 M5.update() 抢 I2C
static volatile bool ntpSynced = false;

static void onTimeSync(struct timeval *tv) {
  (void)tv;
  ntpSynced = true;
}

static void setTimezone(const char *tz) {
  setenv("TZ", tz, 1);
  tzset();
}

static void startSntp() {
  // configTzTime 会先停止已有的 SNTP，再用新的服务器重新开始，并设置时区
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
}

// RTC（UTC）→ 系统时间
static bool loadTimeFromRtc() {
  if (!M5.Rtc.isEnabled()) {
    logger.warn("time", "RTC not found");
    return false;
  }

  tm t = M5.Rtc.getDateTime().get_tm();
  if (t.tm_year + 1900 < VALID_MIN_YEAR) {
    logger.warn("time", "RTC time not set (%04d-%02d-%02d)", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    return false;
  }

  // RTC 存的是 UTC：临时把时区切到 UTC 再用 mktime 换算成时间戳
  setTimezone("UTC0");
  t.tm_isdst = 0;
  time_t now = mktime(&t);
  setTimezone(NTP_TIMEZONE);

  struct timeval tv = {now, 0};
  settimeofday(&tv, nullptr);

  char buf[32];
  mda_time_format(buf, sizeof(buf));
  logger.info("time", "system time loaded from RTC: %s", buf);
  return true;
}

// 系统时间 → RTC（UTC）
static void saveTimeToRtc() {
  if (!M5.Rtc.isEnabled()) {
    return;
  }
  time_t now = time(nullptr);
  tm utc;
  gmtime_r(&now, &utc);
  M5.Rtc.setDateTime(&utc);
}

void mda_ntp_initialize() {
  setTimezone(NTP_TIMEZONE);
  sntp_set_time_sync_notification_cb(onTimeSync);
  loadTimeFromRtc();
}

void mda_ntp_loop() {
  // STA 刚连上路由器时开始对时（每次重新连上都会再对一次）
  static bool lastConnected = false;
  bool connected = wifi_sta_get_status().state == WifiStaState::Connected;
  if (connected && !lastConnected) {
    logger.info("time", "STA connected, start NTP sync (%s)", NTP_SERVER1);
    startSntp(); //发起ntp，时钟同步
  }
  lastConnected = connected;

  if (ntpSynced) {
    ntpSynced = false;
    saveTimeToRtc(); //将时间保存到RTC中
    //char buf[32];
    //mda_time_format(buf, sizeof(buf));
    //logger.info("time", "NTP synced: %s, saved to RTC", buf);
  }
}

bool mda_time_valid() {
  time_t now = time(nullptr);
  tm local;
  localtime_r(&now, &local);
  return local.tm_year + 1900 >= VALID_MIN_YEAR;
}

bool mda_time_format(char *buf, size_t len) {
  if (!mda_time_valid()) {
    return false;
  }
  time_t now = time(nullptr);
  tm local;
  localtime_r(&now, &local);
  strftime(buf, len, "%H:%M:%S", &local);
  return true;
}

bool mda_date_format(char *buf, size_t len) {
  if (!mda_time_valid()) {
    return false;
  }
  time_t now = time(nullptr);
  tm local;
  localtime_r(&now, &local);
  strftime(buf, len, "%Y-%m-%d", &local);
  return true;
}

bool mda_datetime_format(char *buf, size_t len) {
  if (!mda_time_valid()) {
    return false;
  }
  time_t now = time(nullptr);
  tm local;
  localtime_r(&now, &local);
  strftime(buf, len, "%Y-%m-%d %H:%M:%S", &local);
  return true;
}

bool mda_date_format_custom(const char* formt, char *buf, size_t len) {
  if (!mda_time_valid()) {
    return false;
  }
  time_t now = time(nullptr);
  tm local;
  localtime_r(&now, &local);
  strftime(buf, len, formt, &local);
  return true;
}
