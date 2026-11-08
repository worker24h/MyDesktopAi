#include "mda_wifi_config.h"

#include <ArduinoJson.h>
#include <SD.h>

#include "mda_sd_manager.h"

static const char *TMP_PATH = MDA_WIFI_CONFIG_PATH ".tmp";

bool wifi_config_load(WifiConfig &cfg) {
  if (!sd_is_mounted()) {
    return false;
  }
  if (!SD.exists(MDA_WIFI_CONFIG_PATH)) {
    Serial.println("WifiConfig: no config file");
    return false;
  }

  File file = SD.open(MDA_WIFI_CONFIG_PATH, FILE_READ);
  if (!file) {
    Serial.println("WifiConfig: open failed");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) {
    Serial.printf("WifiConfig: parse failed: %s\n", err.c_str());
    return false;
  }

  cfg.ssid = doc["ssid"] | "";
  cfg.password = doc["password"] | "";
  if (cfg.ssid.length() == 0) {
    Serial.println("WifiConfig: ssid is empty");
    return false;
  }
  Serial.printf("WifiConfig: loaded, ssid=\"%s\"\n", cfg.ssid.c_str());
  return true;
}

bool wifi_config_save(const WifiConfig &cfg) {
  if (!sd_is_mounted()) {
    return false;
  }
  if (!sd_init_dirs()) {
    return false;
  }

  JsonDocument doc;
  doc["ssid"] = cfg.ssid;
  doc["password"] = cfg.password;

  // 先完整写入临时文件
  File file = SD.open(TMP_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("WifiConfig: create tmp file failed");
    return false;
  }
  size_t written = serializeJsonPretty(doc, file);
  file.close();
  if (written == 0) {
    Serial.println("WifiConfig: write failed");
    SD.remove(TMP_PATH);
    return false;
  }

  // 再替换正式文件（FAT 上 rename 不能覆盖已存在的文件，需要先删除）
  if (SD.exists(MDA_WIFI_CONFIG_PATH)) {
    SD.remove(MDA_WIFI_CONFIG_PATH);
  }
  if (!SD.rename(TMP_PATH, MDA_WIFI_CONFIG_PATH)) {
    Serial.println("WifiConfig: rename failed");
    return false;
  }
  Serial.printf("WifiConfig: saved, ssid=\"%s\"\n", cfg.ssid.c_str());
  return true;
}

bool wifi_config_validate(const WifiConfig &cfg, String &err) {
  if (cfg.ssid.length() == 0) {
    err = "SSID 不能为空";
    return false;
  }
  if (cfg.ssid.length() > 32) {
    err = "SSID 不能超过 32 字节";
    return false;
  }
  size_t plen = cfg.password.length();
  if (plen > 0 && plen < 8) {
    err = "密码至少 8 位（Open 网络请留空）";
    return false;
  }
  if (plen > 64) {
    err = "密码不能超过 64 位";
    return false;
  }
  return true;
}
