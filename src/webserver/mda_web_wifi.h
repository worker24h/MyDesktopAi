#ifndef MDA_WEB_WIFI_H
#define MDA_WEB_WIFI_H

// WiFi 配置网页，挂在 webfs 的同一个 Web 服务器上：
//   GET  /wifi              配置页面
//   GET  /api/wifi/status   STA 当前状态（JSON）
//   GET  /api/wifi/scan     扫描附近 WiFi（JSON，耗时约 2~4 秒）
//   POST /api/wifi          保存配置并立即重连（表单字段 ssid、password）
// 需要在 webfs_setup() 之后调用。
void web_wifi_setup();

#endif
