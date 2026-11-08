#include "mda_network.h"

#include <WiFi.h>

void network_setup() {
  // ESP32 的 WiFi 是片上集成的，不存在 "shield 不在" 的情况；
  // 扫描前需要先进入 STA 模式，并断开可能存在的旧连接。
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  printMacAddress();
}

static const char *wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    case WL_STOPPED:
      return "WL_STOPPED";
    case WL_NO_SHIELD:
      return "WL_NO_SHIELD";
    default:
      return "UNKNOWN";
  }
}

// 打印连接、断开、拿到 IP 等事件，断开时打印原因（密码错误、找不到网络等）
static void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi: associated with AP");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi: got IP ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      uint8_t reason = info.wifi_sta_disconnected.reason;
      Serial.printf("WiFi: disconnected, reason %u (%s)\n", reason,
                    WiFi.disconnectReasonName((wifi_err_reason_t)reason));
      break;
    }
    default:
      break;
  }
}

bool wifi_connect(const char *ssid, const char *password, uint32_t timeout_ms) {
  static bool eventRegistered = false;
  if (!eventRegistered) {
    WiFi.onEvent(onWiFiEvent);
    eventRegistered = true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.printf("WiFi: connecting to \"%s\" ...\n", ssid);
  WiFi.begin(ssid, password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  wl_status_t status = WiFi.status();
  if (status != WL_CONNECTED) {
    Serial.printf("WiFi: connect failed, status %d (%s)\n", status, wifiStatusName(status));
    return false;
  }

  Serial.print("WiFi: connected, IP ");
  Serial.print(WiFi.localIP());
  Serial.printf(", RSSI %d dBm\n", WiFi.RSSI());
  return true;
}

bool wifi_start_ap(const char *ssid, const char *password) {
  if (password && password[0] != '\0' && strlen(password) < 8) {
    Serial.println("WiFi AP: password must be at least 8 characters");
    return false;
  }
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ssid, (password && password[0]) ? password : nullptr);
  if (!ok) {
    Serial.println("WiFi AP: softAP failed");
    return false;
  }
  Serial.printf("WiFi AP: \"%s\" started, IP ", ssid);
  Serial.println(WiFi.softAPIP());
  return true;
}

void printMacAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  // ESP32 返回的 mac[0] 是最高字节，按正序、补零打印
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void listNetworks() {
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();
  if (numSsid < 0) {  // WIFI_SCAN_FAILED(-2) / WIFI_SCAN_RUNNING(-1)
    Serial.printf("Scan failed: %d\n", numSsid);
    return;
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
  }
}

void printEncryptionType(int thisType) {
  // 打印 wifi_auth_mode_t 的枚举名
  switch (thisType) {
    case WIFI_AUTH_OPEN:
      Serial.println("WIFI_AUTH_OPEN");
      break;
    case WIFI_AUTH_WEP:
      Serial.println("WIFI_AUTH_WEP");
      break;
    case WIFI_AUTH_WPA_PSK:
      Serial.println("WIFI_AUTH_WPA_PSK");
      break;
    case WIFI_AUTH_WPA2_PSK:
      Serial.println("WIFI_AUTH_WPA2_PSK");
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      Serial.println("WIFI_AUTH_WPA_WPA2_PSK");
      break;
    case WIFI_AUTH_ENTERPRISE:  // 与 WIFI_AUTH_WPA2_ENTERPRISE 同值
      Serial.println("WIFI_AUTH_ENTERPRISE");
      break;
    case WIFI_AUTH_WPA3_PSK:
      Serial.println("WIFI_AUTH_WPA3_PSK");
      break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
      Serial.println("WIFI_AUTH_WPA2_WPA3_PSK");
      break;
    case WIFI_AUTH_WAPI_PSK:
      Serial.println("WIFI_AUTH_WAPI_PSK");
      break;
    case WIFI_AUTH_OWE:
      Serial.println("WIFI_AUTH_OWE");
      break;
    case WIFI_AUTH_WPA3_ENT_192:
      Serial.println("WIFI_AUTH_WPA3_ENT_192");
      break;
    case WIFI_AUTH_WPA3_EXT_PSK:
      Serial.println("WIFI_AUTH_WPA3_EXT_PSK");
      break;
    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
      Serial.println("WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE");
      break;
    case WIFI_AUTH_DPP:
      Serial.println("WIFI_AUTH_DPP");
      break;
    case WIFI_AUTH_WPA3_ENTERPRISE:
      Serial.println("WIFI_AUTH_WPA3_ENTERPRISE");
      break;
    case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:
      Serial.println("WIFI_AUTH_WPA2_WPA3_ENTERPRISE");
      break;
    case WIFI_AUTH_WPA_ENTERPRISE:
      Serial.println("WIFI_AUTH_WPA_ENTERPRISE");
      break;
    default:
      Serial.print("UNKNOWN: ");
      Serial.println(thisType);
      break;
  }
}
