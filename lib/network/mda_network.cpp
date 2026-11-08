
#include <WiFi.h>
#include "mda_network.h"
#include "mda_logger.h"

static const uint32_t STA_RETRY_INTERVAL_MS = 15000;  // 失败后多久重试一次
static const uint8_t STA_MAX_ATTEMPTS = 3;             // 连续失败这么多次后停止重试

static String staSsid;
static String staPassword;
static volatile WifiStaState staState = WifiStaState::NotConfigured;
static volatile uint8_t staLastReason = 0;
static volatile uint32_t staFailedAt = 0;
// 本轮已发起的连接次数。在 staBegin() 里计数（而不是按断开事件计数），
// 因为一次连接失败有时会收到多条断开事件
static volatile uint8_t staAttempts = 0;

// WiFi 事件在 WiFi 系统任务里回调，只更新简单的状态变量；日志类是线程安全的
static void on_ap_sta_event(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      staState = WifiStaState::Connected;
      staLastReason = 0;
      staAttempts = 0;  // 连上后清零，之后断线可以再重试 3 次
      logger.info("wifi", "STA connected to \"%s\", ip=%s rssi=%d", WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      uint8_t reason = info.wifi_sta_disconnected.reason;
      // 主动断开（切换配置、重新连接时）产生的 ASSOC_LEAVE / STA_LEAVING 不算失败
      if (reason == WIFI_REASON_ASSOC_LEAVE || reason == WIFI_REASON_STA_LEAVING) {
        break;
      }
      if (staState == WifiStaState::NotConfigured) {
        break;
      }
      staLastReason = reason;
      staFailedAt = millis();
      staState = WifiStaState::Failed;
      // 事件在 WiFi 任务中执行，SSID 取事件自带的数据，不读 staSsid（主任务可能正在修改它）
      char ssid[33] = {0};
      uint8_t len = info.wifi_sta_disconnected.ssid_len;
      if (len > 32) {
        len = 32;
      }
      memcpy(ssid, info.wifi_sta_disconnected.ssid, len);
      logger.warn("wifi", "STA disconnected from \"%s\", reason %u (%s), attempt %u/%u", ssid,
                  reason, WiFi.disconnectReasonName((wifi_err_reason_t)reason),
                  staAttempts, STA_MAX_ATTEMPTS);
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
      logger.info("wifi", "AP: a device joined, clients=%u", WiFi.softAPgetStationNum());
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
      logger.info("wifi", "AP: a device left, clients=%u", WiFi.softAPgetStationNum());
      break;
    }
    default: {
      break;
    }
  }
}

static void sta_begin() {
  staAttempts = staAttempts + 1;
  staState = WifiStaState::Connecting;
  logger.info("wifi", "STA connecting to \"%s\" (%s), attempt %u/%u", staSsid.c_str(),
              staPassword.length() ? "WPA" : "open", staAttempts, STA_MAX_ATTEMPTS);
  if (staPassword.length() > 0) {
    WiFi.begin(staSsid.c_str(), staPassword.c_str());
  } else {
    WiFi.begin(staSsid.c_str());
  }
}

bool wifi_start_ap_sta(const char *apSsid, const char *apPassword) {
  if (apPassword && apPassword[0] != '\0' && strlen(apPassword) < 8) {
    logger.error("wifi", "AP password must be at least 8 characters");
    return false;
  }

  static bool eventRegistered = false;
  if (!eventRegistered) {
    WiFi.onEvent(on_ap_sta_event); //注册回掉地址
    eventRegistered = true;
  }

  WiFi.mode(WIFI_AP_STA);
  // 重连由 wifi_sta_loop() 统一管理，关掉库自带的自动重连，避免两边同时发起连接
  WiFi.setAutoReconnect(false);

  bool ok = WiFi.softAP(apSsid, (apPassword && apPassword[0]) ? apPassword : nullptr);
  if (!ok) {
    logger.error("wifi", "softAP \"%s\" failed", apSsid);
    return false;
  }
  logger.info("wifi", "AP \"%s\" started, ip=%s", apSsid, WiFi.softAPIP().toString().c_str());
  return true;
}

void wifi_sta_connect(const char *ssid, const char *password) {
  // 先断开当前 STA 连接（不关闭 WiFi，不影响 AP）
  WifiStaState prev = staState;
  staState = WifiStaState::NotConfigured;
  if (prev != WifiStaState::NotConfigured) {
    WiFi.disconnect(false, false);
  }

  staSsid = ssid ? ssid : "";
  staPassword = password ? password : "";
  staLastReason = 0;
  staAttempts = 0;  // 新配置重新计数

  if (staSsid.length() == 0) {
    logger.info("wifi", "STA config cleared");
    return;
  }
  sta_begin();
}

void wifi_sta_loop() {
  if (staState != WifiStaState::Failed) {
    return;
  }
  if (staAttempts >= STA_MAX_ATTEMPTS) {
    staState = WifiStaState::Stopped;
    logger.error("wifi", "STA failed %u times, stop retrying. Save the config again or reboot to retry",
                 STA_MAX_ATTEMPTS);
    return;
  }
  if (millis() - staFailedAt < STA_RETRY_INTERVAL_MS) {
    return;
  }
  sta_begin();
}

WifiStaStatus wifi_sta_get_status() {
  WifiStaStatus st;
  st.state = staState;
  st.ssid = staSsid;
  st.rssi = 0;
  st.retryInMs = 0;
  st.attempts = staAttempts;
  st.maxAttempts = STA_MAX_ATTEMPTS;
  st.lastReason = staLastReason;
  st.lastReasonName = staLastReason ? WiFi.disconnectReasonName((wifi_err_reason_t)staLastReason) : "";
  if (st.state == WifiStaState::Connected) {
    st.ip = WiFi.localIP().toString();
    st.rssi = WiFi.RSSI();
  }
  if (st.state == WifiStaState::Failed && staAttempts < STA_MAX_ATTEMPTS) {
    uint32_t elapsed = millis() - staFailedAt;
    st.retryInMs = elapsed < STA_RETRY_INTERVAL_MS ? STA_RETRY_INTERVAL_MS - elapsed : 0;
  }
  return st;
}

const char *wifi_sta_state_name(WifiStaState state) {
  switch (state) {
    case WifiStaState::NotConfigured: {
      return "not_configured";
    }
    case WifiStaState::Connecting: {
      return "connecting";
    }
    case WifiStaState::Connected: {
      return "connected";
    }
    case WifiStaState::Failed: {
      return "failed";
    }
    case WifiStaState::Stopped: {
      return "stopped";
    }
    default: {
      return "unknown";
    }
  }
}
