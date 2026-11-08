#include "mda_webfs.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>

#include "mda_sd_manager.h"
#include "mda_logger.h"

// CoreS3 microSD：CS=4，SPI 总线与屏幕共用（SCK=36, MISO=35, MOSI=37）
static const uint8_t SD_CS_PIN = 4;
static const uint32_t SD_SPI_FREQ = 25000000;

static WebServer server(80);
static bool sdReady = false;

// ---------- 工具函数 ----------

static String htmlEscape(const String &s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c; break;
    }
  }
  return out;
}

// 用于拼接到 URL 查询参数里（中文文件名按 UTF-8 字节编码）
static String urlEncode(const String &s) {
  static const char hex[] = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
      out += (char)c;
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

static String formatSize(uint64_t bytes) {
  char buf[32];
  if (bytes < 1024) {
    snprintf(buf, sizeof(buf), "%llu B", bytes);
  } else if (bytes < 1024ULL * 1024) {
    snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
  } else if (bytes < 1024ULL * 1024 * 1024) {
    snprintf(buf, sizeof(buf), "%.1f MB", bytes / 1024.0 / 1024.0);
  } else {
    snprintf(buf, sizeof(buf), "%.2f GB", bytes / 1024.0 / 1024.0 / 1024.0);
  }
  return String(buf);
}

// 规范化路径：必须以 / 开头，禁止 ".." 跳出根目录
static bool normalizePath(String path, String &out) {
  if (path.length() == 0) {
    path = "/";
  }
  if (path[0] != '/') {
    path = "/" + path;
  }
  if (path.indexOf("..") >= 0) {
    return false;
  }
  while (path.length() > 1 && path.endsWith("/")) {
    path.remove(path.length() - 1);
  }
  out = path;
  return true;
}

static String parentPath(const String &path) {
  int idx = path.lastIndexOf('/');
  if (idx <= 0) {
    return "/";
  }
  return path.substring(0, idx);
}

static String contentTypeFor(const String &name) {
  String n = name;
  n.toLowerCase();
  if (n.endsWith(".htm") || n.endsWith(".html")) {
    return "text/html; charset=utf-8";
  }
  if (n.endsWith(".txt") || n.endsWith(".log") || n.endsWith(".csv") || n.endsWith(".md") ||
      n.endsWith(".ini") || n.endsWith(".cfg") || n.endsWith(".yaml") || n.endsWith(".yml")) {
    return "text/plain; charset=utf-8";
  }
  if (n.endsWith(".json")) {
    return "application/json";
  }
  if (n.endsWith(".css")) {
    return "text/css";
  }
  if (n.endsWith(".js")) {
    return "application/javascript";
  }
  if (n.endsWith(".png")) {
    return "image/png";
  }
  if (n.endsWith(".jpg") || n.endsWith(".jpeg")) {
    return "image/jpeg";
  }
  if (n.endsWith(".gif")) {
    return "image/gif";
  }
  if (n.endsWith(".bmp")) {
    return "image/bmp";
  }
  if (n.endsWith(".svg")) {
    return "image/svg+xml";
  }
  if (n.endsWith(".mp3")) {
    return "audio/mpeg";
  }
  if (n.endsWith(".wav")) {
    return "audio/wav";
  }
  if (n.endsWith(".mp4")) {
    return "video/mp4";
  }
  if (n.endsWith(".pdf")) {
    return "application/pdf";
  }
  return "application/octet-stream";
}

static const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CoreS3 SD</title>
<style>
:root{--bg:#fff;--fg:#1d1d1f;--muted:#6e6e73;--line:#e5e5ea;--link:#0066cc;--hover:#f5f5f7}
@media (prefers-color-scheme:dark){:root{--bg:#1c1c1e;--fg:#f5f5f7;--muted:#98989d;--line:#38383a;--link:#4da3ff;--hover:#2c2c2e}}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.5 -apple-system,BlinkMacSystemFont,"PingFang SC",sans-serif}
main{max-width:900px;margin:0 auto;padding:16px}
h1{font-size:20px;margin:8px 0}
.path{color:var(--muted);word-break:break-all;margin-bottom:12px}
table{width:100%;border-collapse:collapse}
td{padding:8px 6px;border-bottom:1px solid var(--line);vertical-align:top}
tr:hover td{background:var(--hover)}
td.name{word-break:break-all}
td.size,td.op{white-space:nowrap;text-align:right;color:var(--muted)}
a{color:var(--link);text-decoration:none}
a:hover{text-decoration:underline}
.foot{color:var(--muted);margin-top:16px;font-size:13px}
.danger{margin-top:32px;padding:12px 14px;border:1px solid #d70015;border-radius:8px}
.danger h2{font-size:15px;margin:0 0 6px;color:#d70015}
.danger p{margin:0 0 10px;color:var(--muted);font-size:13px}
.danger input{font:inherit;padding:6px 8px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--fg);width:140px}
.danger button{font:inherit;padding:6px 14px;margin-left:6px;border:0;border-radius:6px;background:#d70015;color:#fff;cursor:pointer}
@media (prefers-color-scheme:dark){.danger{border-color:#ff453a}.danger h2{color:#ff453a}.danger button{background:#ff453a}}
</style></head><body><main>
)HTML";

static const char PAGE_TAIL[] PROGMEM = "</main></body></html>";

// 首页底部的格式化区域。需要输入 FORMAT 并再次弹窗确认，避免误操作。
static const char FORMAT_FORM[] PROGMEM = R"HTML(
<form class="danger" method="POST" action="/format"
  onsubmit="return this.confirm.value==='FORMAT' && window.confirm('确定要格式化 SD 卡吗？所有文件都会被删除，无法恢复！')">
<h2>格式化 SD 卡</h2>
<p>将 SD 卡格式化为 FAT32，卡上所有文件都会被删除且无法恢复。请输入 FORMAT 确认。</p>
<input name="confirm" placeholder="FORMAT" autocomplete="off"><button type="submit">格式化</button>
</form>
)HTML";

// ---------- 请求处理 ----------

// GET /?path=/dir  列出目录
static void handleList() {
  String path;
  if (!normalizePath(server.arg("path"), path)) {
    server.send(400, "text/plain; charset=utf-8", "非法路径");
    return;
  }

  if (!sdReady) {
    server.send(503, "text/plain; charset=utf-8", "SD 卡未就绪：请检查是否插卡，然后重启设备");
    return;
  }

  File dir = SD.open(path);
  if (!dir) {
    server.send(404, "text/plain; charset=utf-8", "目录不存在：" + path);
    return;
  }
  if (!dir.isDirectory()) {
    dir.close();
    // 是文件就直接跳转到查看
    server.sendHeader("Location", "/file?path=" + urlEncode(path));
    server.send(302);
    return;
  }

  // 分块发送，目录里文件很多也不会占用大量内存
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent_P(PAGE_HEAD);
  server.sendContent("<h1>CoreS3 SD 卡</h1><div class=\"path\">" + htmlEscape(path) + "</div><table>");

  if (path != "/") {
    server.sendContent("<tr><td class=\"name\"><a href=\"/?path=" + urlEncode(parentPath(path)) +
                       "\">⬆️ 上一级</a></td><td class=\"size\"></td><td class=\"op\"></td></tr>");
  }

  int count = 0;
  String row;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    String name = entry.name();  // 只有文件名，不含路径
    String full = (path == "/") ? "/" + name : path + "/" + name;
    String enc = urlEncode(full);
    String esc = htmlEscape(name);

    if (entry.isDirectory()) {
      row = "<tr><td class=\"name\"><a href=\"/?path=" + enc + "\">📁 " + esc +
            "/</a></td><td class=\"size\"></td><td class=\"op\"></td></tr>";
    } else {
      row = "<tr><td class=\"name\"><a href=\"/file?path=" + enc + "\" target=\"_blank\">📄 " + esc +
            "</a></td><td class=\"size\">" + formatSize(entry.size()) +
            "</td><td class=\"op\"><a href=\"/file?path=" + enc + "&dl=1\">下载</a></td></tr>";
    }
    entry.close();
    server.sendContent(row);
    count++;
  }
  dir.close();

  if (count == 0) {
    server.sendContent("<tr><td colspan=\"3\" class=\"size\" style=\"text-align:left\">（空目录）</td></tr>");
  }

  uint64_t used = SD.usedBytes();
  uint64_t total = SD.totalBytes();
  server.sendContent("</table><div class=\"foot\">" + String(count) + " 项 · 已用 " + formatSize(used) +
                     " / 共 " + formatSize(total) + "</div>");
  if (path == "/") {
    server.sendContent_P(FORMAT_FORM);
  }
  server.sendContent_P(PAGE_TAIL);
  server.sendContent("");  // 结束分块传输
}

// GET /file?path=/a.txt[&dl=1]  查看或下载文件
static void handleFile() {
  String path;
  if (!normalizePath(server.arg("path"), path) || path == "/") {
    server.send(400, "text/plain; charset=utf-8", "非法路径");
    return;
  }
  if (!sdReady) {
    server.send(503, "text/plain; charset=utf-8", "SD 卡未就绪");
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) {
      file.close();
    }
    server.send(404, "text/plain; charset=utf-8", "文件不存在：" + path);
    return;
  }

  String name = path.substring(path.lastIndexOf('/') + 1);
  bool download = server.hasArg("dl");
  String type = download ? String("application/octet-stream") : contentTypeFor(name);
  if (download) {
    // filename* 支持中文文件名
    server.sendHeader("Content-Disposition", "attachment; filename*=UTF-8''" + urlEncode(name));
  }

  Serial.printf("WebFS: send %s (%u bytes)\n", path.c_str(), (unsigned)file.size());
  server.streamFile(file, type);
  file.close();
}

// POST /format  confirm=FORMAT  格式化 SD 卡
static void handleFormat() {
  if (server.arg("confirm") != "FORMAT") {
    server.send(400, "text/plain; charset=utf-8", "未确认：请在输入框中输入 FORMAT 后再提交");
    return;
  }

  Serial.println("WebFS: format requested from web page");
  // 格式化前先关闭日志文件，避免日志写入和格式化同时操作 SD 卡
  logger.suspend();
  bool ok = sd_format();
  sdReady = ok;
  logger.resume();  // 重新创建 /data/logs 并打开日志文件
  logger.warn("webfs", "SD card formatted from web page, result=%s", ok ? "ok" : "failed");

  String msg = ok ? "格式化完成，SD 卡已重新挂载。" : "格式化失败，请查看串口日志。";
  String html = FPSTR(PAGE_HEAD);
  html += "<h1>" + msg + "</h1><p><a href=\"/\">返回文件列表</a></p>";
  html += FPSTR(PAGE_TAIL);
  server.send(ok ? 200 : 500, "text/html; charset=utf-8", html);
}

static void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "Not found");
}

// ---------- 对外接口 ----------

bool webfs_setup() {
  SPI.begin(SCK, MISO, MOSI, SD_CS_PIN);
  sdReady = SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ);
  if (sdReady) {
    Serial.printf("WebFS: SD mounted, %s total\n", formatSize(SD.totalBytes()).c_str());
  } else {
    Serial.println("WebFS: SD mount failed (没插卡或格式不是 FAT32/exFAT?)");
  }

  server.on("/", HTTP_GET, handleList);
  server.on("/file", HTTP_GET, handleFile);
  server.on("/format", HTTP_POST, handleFormat);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("WebFS: HTTP server started on port 80");
  return sdReady;
}

void webfs_loop() {
  server.handleClient();
}
