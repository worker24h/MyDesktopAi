#include <M5Unified.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "mda_network.h"
#include "mda_sdmsc.h"
#include "mda_sd_manager.h"
#include "mda_webfs.h"
#include "mda_logger.h"

// CoreS3 自己开的 WiFi 热点。密码至少 8 位，请改成你自己的。
static const char *AP_SSID = "CoreS3-SD";
static const char *AP_PASSWORD = "cores3sd";

void setup() {
    M5.begin();
    Serial.begin(115200);
    int count = 3; 
    while (!Serial && count > 0) {
      ; // 最多等 3 秒串口连接，超时后继续运行
      delay(1000);
      M5.Display.printf("Initial CoreS3, waiting for Serial, %d",count--);
    }

    M5.Display.setTextSize(2);
    M5.Display.println("Hello CoreS3");

    network_setup();
    listNetworks();

    if (wifi_start_ap(AP_SSID, AP_PASSWORD)) {
        M5.Display.printf("WiFi: %s\n", AP_SSID);
        M5.Display.printf("Pass: %s\n", AP_PASSWORD);
        M5.Display.print("http://");
        M5.Display.println(WiFi.softAPIP());
    } else {
        M5.Display.println("WiFi AP failed");
    }

    if (!webfs_setup()) {
        M5.Display.println("SD mount failed");
    }

    // 日志写入 SD 卡 /data/logs/app.log，必须在 SD 卡挂载（webfs_setup）之后启动
    logger.initialize();
    logger.info("main", "setup done, AP=%s ip=%s", AP_SSID, WiFi.softAPIP().toString().c_str());

    //sd_setup();
}

void loop() {
    M5.update();
    webfs_loop();
}
