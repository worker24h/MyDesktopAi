#ifndef MDA_WIFI_CONFIG_H
#define MDA_WIFI_CONFIG_H

#include <Arduino.h>

// STA（连接路由器）用的 WiFi 配置，保存在 SD 卡 /data/config/wifi.json：
//   { "ssid": "MyWiFi", "password": "xxxx" }
// 密码为空表示 Open 网络。密码以明文保存。

#define MDA_WIFI_CONFIG_PATH "/data/config/wifi.json"

struct WifiConfig {
  String ssid;
  String password;
};

// 读取配置。文件不存在、格式错误或 ssid 为空时返回 false。
bool wifi_config_load(WifiConfig &cfg);

// 保存配置。先写临时文件再替换，避免写到一半断电导致配置损坏。成功返回 true。
bool wifi_config_save(const WifiConfig &cfg);

// 检查配置是否合法：ssid 1~32 字节；password 为空（Open 网络）或 8~64 字节。
// 不合法时返回 false，并把原因写入 err。
bool wifi_config_validate(const WifiConfig &cfg, String &err);

#endif
