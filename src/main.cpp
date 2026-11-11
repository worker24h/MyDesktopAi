#include <M5Unified.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "mda_network.h"
#include "mda_sd_manager.h"
#include "webserver/mda_webfs.h"
#include "webserver/mda_web_wifi.h"
#include "config/mda_wifi_config.h"
#include "mda_logger.h"
#include "mda_ntp.h"
#include "hal/mda_lvgl.h"
#include "ui/mda_ui.h"

// CoreS3 自己开的 WiFi 热点（管理入口）。密码至少 8 位，请改成你自己的。
static const char *AP_SSID = "CoreS3-SD";
static const char *AP_PASSWORD = "cores3sd";


/**
 * 配置项目
 */
void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.led_brightness = 200;
  M5.begin(cfg);

  int count = 3;
  while (!Serial && count > 0) {
    ; // 最多等 3 秒串口连接，超时后继续运行
    delay(1000);
    M5.Display.printf("Initial CoreS3, waiting for Serial, %d", count--);
  }

  // 1. 存储：挂载 SD 卡，初始化数据目录 /data、/data/config、/data/logs
  if (sd_setup()) {
    sd_init_dirs();
  }

  // 2. 日志：写入 /data/logs/app.log（SD 不可用时只输出串口）
  logger.initialize();
  logger.info("main", "boot");

  // 时间：RTC → 系统时间，设置时区（UTC+8）；STA 连上后会自动 NTP 对时
  mda_ntp_initialize();


  // 3. WiFi：AP 始终开启；有 STA 配置就去连接路由器
  wifi_start_ap_sta(AP_SSID, AP_PASSWORD);
  WifiConfig wifiCfg;
  if (wifi_config_load(wifiCfg)) {
    wifi_sta_connect(wifiCfg.ssid.c_str(), wifiCfg.password.c_str());
  } else {
    logger.info("main", "no STA config, open http://%s/wifi to set up",
          WiFi.softAPIP().toString().c_str());
  }

  // 4. Web：文件浏览（/）+ WiFi 设置（/wifi）
  webfs_setup();
  web_wifi_setup();

  Serial.printf("芯片: %s，%d 核，%d MHz\n", ESP.getChipModel(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Serial.printf("内部 SRAM 剩余: %u KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("PSRAM 总共: %u KB，剩余: %u KB\n", ESP.getPsramSize() / 1024, ESP.getFreePsram() / 1024);
  Serial.printf("Flash: %u MB\n", ESP.getFlashChipSize() / 1024 / 1024);

  mda_lvgl_initilaized();
  ui_init();

  logger.info("main", "setup done");
}

/**
 * 循环处理
 */
void loop() {
  M5.update();
  webfs_loop();
  wifi_sta_loop();
  mda_ntp_loop();
  mda_lvgl_loop();

}
