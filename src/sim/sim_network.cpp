#include "mda_network.h"
#include "mda_ntp.h"
#include <time.h>
#include <stdio.h>

// ---------------- 仿真 WiFi 实现 ----------------

bool wifi_start_ap_sta(const char *apSsid, const char *apPassword) {
    (void)apSsid;
    (void)apPassword;
    return true;
}

void wifi_sta_connect(const char *ssid, const char *password) {
    (void)ssid;
    (void)password;
}

void wifi_sta_loop() {
    // 仿真环境下无需轮询硬件 WiFi
}

WifiStaStatus wifi_sta_get_status() {
    struct WifiStaStatus status;
    status.state = Connected;
    status.ssid = "sim-wifi";
    status.ip = "127.0.0.1";
    status.rssi = 90;
    status.lastReason = 0;
    status.lastReasonName = "";
    status.retryInMs = 0;
    status.attempts = 1;
    status.maxAttempts = 3;
    return status;
}

const char *wifi_sta_state_name(WifiStaState state) {
    switch (state) {
        case NotConfigured: return "not_configured";
        case Connecting:    return "connecting";
        case Connected:     return "connected";
        case Failed:        return "failed";
        case Stopped:       return "stopped";
        default:            return "unknown";
    }
}

// ---------------- 仿真 NTP / 时间实现（直接读取 PC 系统时间） ----------------

void mda_ntp_initialize() {
}

void mda_ntp_loop() {
}

bool mda_time_valid() {
    return true;
}

bool mda_time_format(char *buf, size_t len) {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    return strftime(buf, len, "%H:%M:%S", &tm_info) > 0;
}

bool mda_datetime_format(char *buf, size_t len) {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    return strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm_info) > 0;
}

bool mda_date_format(char *buf, size_t len) {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    return strftime(buf, len, "%Y-%m-%d", &tm_info) > 0;
}

bool mda_date_format_custom(const char* format, char *buf, size_t len) {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    return strftime(buf, len, format, &tm_info) > 0;
}
