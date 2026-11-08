#include <M5Unified.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include "mda_network.h"
#include "mda_sd_manager.h"
#include "webserver/mda_webfs.h"
#include "webserver/mda_web_wifi.h"
#include "config/mda_wifi_config.h"
#include "mda_logger.h"

// CoreS3 自己开的 WiFi 热点（管理入口）。密码至少 8 位，请改成你自己的。
static const char *AP_SSID = "CoreS3-SD";
static const char *AP_PASSWORD = "cores3sd";

// 屏幕上显示 AP 和 STA 的状态，STA 状态变化时重画
static void drawStatus() {
    WifiStaStatus st = wifi_sta_get_status();

    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(0, 0);
    M5.Display.setTextSize(2);
    M5.Display.println("MyDesktopAi");
    M5.Display.println();

    M5.Display.println("[AP]");
    M5.Display.printf("SSID: %s\n", AP_SSID);
    M5.Display.printf("Pass: %s\n", AP_PASSWORD);
    M5.Display.print("http://");
    M5.Display.println(WiFi.softAPIP());
    M5.Display.println();

    M5.Display.println("[STA]");
    switch (st.state) {
        case WifiStaState::NotConfigured: {
            M5.Display.println("not configured");
            M5.Display.println("open /wifi to set up");
            break;
        }
        case WifiStaState::Connecting: {
            M5.Display.printf("%s\n", st.ssid.c_str());
            M5.Display.println("connecting...");
            break;
        }
        case WifiStaState::Connected: {
            M5.Display.printf("%s\n", st.ssid.c_str());
            M5.Display.printf("IP: %s\n", st.ip.c_str());
            break;
        }
        case WifiStaState::Stopped: {
            M5.Display.printf("%s\n", st.ssid.c_str());
            M5.Display.printf("stopped: %s\n", st.lastReasonName.c_str());
            M5.Display.println("re-save on /wifi to retry");
            break;
        }
        case WifiStaState::Failed: {
            M5.Display.printf("%s\n", st.ssid.c_str());
            M5.Display.printf("failed: %s\n", st.lastReasonName.c_str());
            break;
        }
    }

    if (!sd_is_mounted()) {
        M5.Display.println();
        M5.Display.println("SD card not ready");
    }
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    int count = 3;
    while (!Serial && count > 0) {
        ; // 最多等 3 秒串口连接，超时后继续运行
        delay(1000);
        M5.Display.printf("Initial CoreS3, waiting for Serial, %d", count--);
    }

    // 1. 存储：挂载 SD 卡，初始化数据目录 /data、/data/config、/data/logs
    if (sd_mount()) {
        sd_init_dirs();
    }

    // 2. 日志：写入 /data/logs/app.log（SD 不可用时只输出串口）
    logger.initialize();
    logger.info("main", "boot");

    listNetworks();
    
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

    drawStatus();
    logger.info("main", "setup done");
}

void loop() {
    M5.update();
    webfs_loop();
    wifi_sta_loop();

    // STA 状态或 IP 变化时刷新屏幕
    static WifiStaState lastState = WifiStaState::NotConfigured;
    static String lastIp;
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck >= 500) {
        lastCheck = millis();
        WifiStaStatus st = wifi_sta_get_status();
        if (st.state != lastState || st.ip != lastIp) {
            lastState = st.state;
            lastIp = st.ip;
            drawStatus();
        }
    }
}
