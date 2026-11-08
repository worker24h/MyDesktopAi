#ifndef MDA_NETWORK_H
#define MDA_NETWORK_H

#include <stdint.h>
#include <Arduino.h>

// 在这里填写要连接的 WiFi。注意：ESP32 只支持 2.4GHz WiFi，不支持 5GHz。
// 不要把这个文件提交到公开仓库。
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"


// 连接 WiFi，最多等待 timeout_ms 毫秒。成功返回 true。
// 连接成功后开启自动重连，断线时会自动重新连接。
bool wifi_connect(const char *ssid, const char *password, uint32_t timeout_ms = 20000);

// 让 CoreS3 自己开一个 WiFi 热点（AP 模式），设备 IP 固定为 192.168.4.1。
// password 至少 8 位（WPA2）；传 nullptr 或 "" 则为无密码热点。成功返回 true。
bool wifi_start_ap(const char *ssid, const char *password);

// ---------------- AP + STA 并存 ----------------
// AP：始终开启的热点，作为管理入口（192.168.4.1）。
// STA：用配置去连接路由器，连不上或断线时自动重试，不影响 AP。
// 注意：芯片只有一套射频，STA 连上路由器后 AP 会切到路由器的信道，热点上的设备可能短暂断开重连。

enum class WifiStaState : uint8_t {
  NotConfigured,  // 没有 STA 配置
  Connecting,     // 正在连接
  Connected,      // 已连接并拿到 IP
  Failed,         // 连接失败或断线，等待自动重试
  Stopped,        // 连续 3 次连接失败，不再重试（重新保存配置或重启后再试）
};

struct WifiStaStatus {
  WifiStaState state;
  String ssid;          // 配置的 SSID
  String ip;            // 已连接时的 IP
  int rssi;             // 已连接时的信号强度 dBm
  uint8_t lastReason;   // 最近一次断开原因码（wifi_err_reason_t），0 表示无
  String lastReasonName;
  uint32_t retryInMs;   // Failed 状态下距下次重试的毫秒数
  uint8_t attempts;     // 本轮已尝试连接的次数（连上后清零）
  uint8_t maxAttempts;  // 最多尝试次数
};

// 以 AP+STA 模式启动 WiFi 并开启热点。成功返回 true。
bool wifi_start_ap_sta(const char *apSsid, const char *apPassword);

// 用新配置连接路由器（不阻塞，立即返回）。ssid 为空则断开 STA 并清除配置。password 为空表示 Open 网络。
void wifi_sta_connect(const char *ssid, const char *password);

// 处理 STA 自动重试，需要在 loop() 中反复调用。连续 3 次失败后停止重试。
void wifi_sta_loop();

// 获取 STA 当前状态
WifiStaStatus wifi_sta_get_status();

const char *wifi_sta_state_name(WifiStaState state);

#endif
