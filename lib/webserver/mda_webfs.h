#ifndef MDA_WEBFS_H
#define MDA_WEBFS_H

// 通过网页浏览、下载 SD 卡上的文件。
// 用法：WiFi（热点或 STA）就绪后调用 webfs_setup()，然后在 loop() 里调用 webfs_loop()。

// 挂载 SD 卡并启动 80 端口的 Web 服务器。SD 卡挂载失败返回 false（服务器仍会启动，页面提示无卡）。
bool webfs_setup();

// 处理网页请求，需要在 loop() 中反复调用。
void webfs_loop();

#endif
