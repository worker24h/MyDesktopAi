#ifndef MDA_NETWORK_H
#define MDA_NETWORK_H

#include <stdint.h>

// 在这里填写要连接的 WiFi。注意：ESP32 只支持 2.4GHz WiFi，不支持 5GHz。
// 不要把这个文件提交到公开仓库。
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"


void network_setup();
void printMacAddress();
void listNetworks();
void printEncryptionType(int thisType);

// 连接 WiFi，最多等待 timeout_ms 毫秒。成功返回 true。
// 连接成功后开启自动重连，断线时会自动重新连接。
bool wifi_connect(const char *ssid, const char *password, uint32_t timeout_ms = 20000);

// 让 CoreS3 自己开一个 WiFi 热点（AP 模式），设备 IP 固定为 192.168.4.1。
// password 至少 8 位（WPA2）；传 nullptr 或 "" 则为无密码热点。成功返回 true。
bool wifi_start_ap(const char *ssid, const char *password);

#endif
