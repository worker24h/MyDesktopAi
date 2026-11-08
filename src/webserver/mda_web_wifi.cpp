#include "mda_web_wifi.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "mda_logger.h"
#include "mda_network.h"
#include "mda_sd_manager.h"
#include "mda_webfs.h"
#include "config/mda_wifi_config.h"

static const char WIFI_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi 设置</title>
<style>
:root{--bg:#fff;--fg:#1d1d1f;--muted:#6e6e73;--line:#e5e5ea;--link:#0066cc;--hover:#f5f5f7;--ok:#248a3d;--err:#d70015;--warn:#b25000}
@media (prefers-color-scheme:dark){:root{--bg:#1c1c1e;--fg:#f5f5f7;--muted:#98989d;--line:#38383a;--link:#4da3ff;--hover:#2c2c2e;--ok:#30d158;--err:#ff453a;--warn:#ff9f0a}}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.5 -apple-system,BlinkMacSystemFont,"PingFang SC",sans-serif}
main{max-width:640px;margin:0 auto;padding:16px}
h1{font-size:20px;margin:8px 0}
h2{font-size:16px;margin:24px 0 8px}
a{color:var(--link);text-decoration:none}
.card{border:1px solid var(--line);border-radius:10px;padding:12px 14px}
.row{display:flex;justify-content:space-between;gap:12px;padding:4px 0}
.row span:first-child{color:var(--muted)}
.ok{color:var(--ok)}.err{color:var(--err)}.warn{color:var(--warn)}
label{display:block;margin:10px 0 4px;color:var(--muted);font-size:13px}
input{box-sizing:border-box;width:100%;font:inherit;padding:8px 10px;border:1px solid var(--line);border-radius:8px;background:var(--bg);color:var(--fg)}
button{font:inherit;padding:8px 16px;border:0;border-radius:8px;background:var(--link);color:#fff;cursor:pointer}
button:disabled{opacity:.5;cursor:default}
button.sub{background:transparent;color:var(--link);padding:4px 0}
.actions{margin-top:14px;display:flex;align-items:center;gap:12px}
#msg{font-size:13px}
ul{list-style:none;margin:0;padding:0}
li{display:flex;justify-content:space-between;gap:8px;padding:8px 6px;border-bottom:1px solid var(--line);cursor:pointer}
li:hover{background:var(--hover)}
li .meta{color:var(--muted);white-space:nowrap}
.name{word-break:break-all}
.hint{color:var(--muted);font-size:13px}
</style></head><body><main>
<p><a href="/">← 文件浏览</a></p>
<h1>WiFi 设置</h1>

<h2>连接状态</h2>
<div class="card">
  <div class="row"><span>热点（AP）</span><span id="ap">-</span></div>
  <div class="row"><span>路由器（STA）</span><span id="state">-</span></div>
  <div class="row"><span>SSID</span><span id="ssid" class="name">-</span></div>
  <div class="row"><span>IP</span><span id="ip">-</span></div>
  <div class="row"><span>信号</span><span id="rssi">-</span></div>
  <div class="row" id="reasonRow" hidden><span>原因</span><span id="reason" class="err"></span></div>
</div>

<h2>连接路由器</h2>
<form id="form" class="card" autocomplete="off">
  <label for="f-ssid">SSID</label>
  <input id="f-ssid" name="ssid" maxlength="32" required>
  <label for="f-pass">密码（Open 网络请留空）</label>
  <input id="f-pass" name="password" type="password" maxlength="64">
  <div class="actions"><button id="save" type="submit">保存并连接</button><span id="msg"></span></div>
  <p class="hint">配置保存在 SD 卡 /data/config/wifi.json。只支持 2.4GHz WiFi。</p>
</form>

<h2>附近的 WiFi <button id="scan" class="sub" type="button">扫描</button></h2>
<ul id="list"><li class="hint" style="cursor:default">点“扫描”查找附近的 WiFi，点击一项可填入 SSID</li></ul>

<script>
const $ = id => document.getElementById(id);
const STATE = {
  not_configured: ['未配置', ''],
  connecting: ['连接中…', 'warn'],
  connected: ['已连接', 'ok'],
  failed: ['连接失败，稍后自动重试', 'err'],
  stopped: ['连续 3 次失败，已停止重试', 'err'],
};

async function refresh() {
  try {
    const r = await fetch('/api/wifi/status');
    const s = await r.json();
    $('ap').textContent = s.ap_ssid + ' · ' + s.ap_ip + ' · ' + s.ap_clients + ' 台设备';
    const [text, cls] = STATE[s.state] || [s.state, ''];
    $('state').textContent = text;
    $('state').className = cls;
    $('ssid').textContent = s.ssid || '-';
    $('ip').textContent = s.ip || '-';
    $('rssi').textContent = s.state === 'connected' ? s.rssi + ' dBm' : '-';
    const showReason = (s.state === 'failed' || s.state === 'stopped') && s.reason;
    $('reasonRow').hidden = !showReason;
    if (showReason) {
      let note = '';
      if (s.state === 'failed' && s.retry_in_ms > 0) {
        note = '（第 ' + s.attempts + '/' + s.max_attempts + ' 次，' + Math.ceil(s.retry_in_ms / 1000) + ' 秒后重试）';
      } else if (s.state === 'stopped') {
        note = '（重新保存配置或重启后再试）';
      }
      $('reason').textContent = s.reason + note;
    }
    if (!$('f-ssid').value && s.ssid) {
      $('f-ssid').value = s.ssid;
    }
  } catch (e) {
    $('state').textContent = '无法获取状态';
    $('state').className = 'err';
  }
}

$('form').addEventListener('submit', async ev => {
  ev.preventDefault();
  $('save').disabled = true;
  $('msg').textContent = '保存中…';
  $('msg').className = '';
  try {
    const body = new URLSearchParams({ssid: $('f-ssid').value, password: $('f-pass').value});
    const r = await fetch('/api/wifi', {method: 'POST', body});
    const res = await r.json();
    $('msg').textContent = res.ok ? '已保存，正在连接…' : res.error;
    $('msg').className = res.ok ? 'ok' : 'err';
    if (res.ok) {
      $('f-pass').value = '';
    }
  } catch (e) {
    $('msg').textContent = '保存失败：' + e;
    $('msg').className = 'err';
  }
  $('save').disabled = false;
  refresh();
});

$('scan').addEventListener('click', async () => {
  const list = $('list');
  $('scan').disabled = true;
  list.innerHTML = '<li class="hint" style="cursor:default">扫描中，约需几秒…</li>';
  try {
    const r = await fetch('/api/wifi/scan');
    const res = await r.json();
    list.innerHTML = '';
    if (!res.ok) {
      list.innerHTML = '<li class="hint" style="cursor:default"></li>';
      list.firstChild.textContent = res.error;
    } else if (res.networks.length === 0) {
      list.innerHTML = '<li class="hint" style="cursor:default">没有找到 WiFi</li>';
    }
    for (const n of (res.networks || [])) {
      const li = document.createElement('li');
      const name = document.createElement('span');
      name.className = 'name';
      name.textContent = (n.open ? '' : '🔒 ') + n.ssid;
      const meta = document.createElement('span');
      meta.className = 'meta';
      meta.textContent = n.rssi + ' dBm · 信道 ' + n.channel;
      li.append(name, meta);
      li.addEventListener('click', () => {
        $('f-ssid').value = n.ssid;
        $('f-pass').value = '';
        $('f-pass').focus();
      });
      list.append(li);
    }
  } catch (e) {
    list.innerHTML = '<li class="hint" style="cursor:default">扫描失败</li>';
  }
  $('scan').disabled = false;
});

refresh();
setInterval(refresh, 2000);
</script>
</main></body></html>
)HTML";

static void sendJson(int code, JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  webfs_server().send(code, "application/json; charset=utf-8", body);
}

static void sendError(int code, const String &error) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = error;
  sendJson(code, doc);
}

// GET /wifi
static void handlePage() {
  webfs_server().send_P(200, "text/html; charset=utf-8", WIFI_PAGE);
}

// GET /api/wifi/status
static void handleStatus() {
  WifiStaStatus st = wifi_sta_get_status();
  JsonDocument doc;
  doc["ap_ssid"] = WiFi.softAPSSID();
  doc["ap_ip"] = WiFi.softAPIP().toString();
  doc["ap_clients"] = WiFi.softAPgetStationNum();
  doc["state"] = wifi_sta_state_name(st.state);
  doc["ssid"] = st.ssid;
  doc["ip"] = st.ip;
  doc["rssi"] = st.rssi;
  doc["reason"] = st.lastReasonName;
  doc["retry_in_ms"] = st.retryInMs;
  doc["attempts"] = st.attempts;
  doc["max_attempts"] = st.maxAttempts;
  doc["sd"] = sd_is_mounted();
  sendJson(200, doc);
}

// GET /api/wifi/scan
static void handleScan() {
  // 同步扫描，期间会短暂切换信道，热点上的设备可能卡顿 1~2 秒
  int n = WiFi.scanNetworks(false, false);
  if (n < 0) {
    // STA 正在连接时扫描会失败
    sendError(503, "WiFi 正忙（可能正在连接路由器），请稍后再扫描");
    return;
  }

  JsonDocument doc;
  doc["ok"] = true;
  JsonArray arr = doc["networks"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;  // 跳过隐藏网络
    }
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = WiFi.RSSI(i);
    o["channel"] = WiFi.channel(i);
    o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  sendJson(200, doc);
}

// POST /api/wifi  ssid=...&password=...
static void handleSave() {
  WebServer &server = webfs_server();
  WifiConfig cfg;
  cfg.ssid = server.arg("ssid");
  cfg.password = server.arg("password");

  String err;
  if (!wifi_config_validate(cfg, err)) {
    sendError(400, err);
    return;
  }
  if (!sd_is_mounted()) {
    sendError(503, "SD 卡不可用，无法保存配置");
    return;
  }
  if (!wifi_config_save(cfg)) {
    sendError(500, "写入 SD 卡失败");
    return;
  }

  logger.info("web", "WiFi config saved from web page, ssid=\"%s\"", cfg.ssid.c_str());
  // 先回复浏览器再重连：STA 重连可能让 AP 切换信道，浏览器连接会短暂中断
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
  delay(100);
  wifi_sta_connect(cfg.ssid.c_str(), cfg.password.c_str());
}

void web_wifi_setup() {
  WebServer &server = webfs_server();
  server.on("/wifi", HTTP_GET, handlePage);
  server.on("/api/wifi/status", HTTP_GET, handleStatus);
  server.on("/api/wifi/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_POST, handleSave);
}
