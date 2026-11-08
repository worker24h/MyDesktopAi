#ifndef MDA_LOGGER_H
#define MDA_LOGGER_H

#include <Arduino.h>
#include <FS.h>

#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// 线程安全的 SD 卡日志类。
//
// 设计：任意任务调用 info()/error() 等时，在调用者自己的任务里把一整行格式化好，
// 放入 FreeRTOS 队列；只有一个后台写入任务从队列取出并写文件。
// 因此：
//   - 多任务并发写日志，每一行都完整写入，不会交错、不会写乱行；
//   - 调用方不会被慢速 SD 卡阻塞（队列满时丢弃并计数，之后在日志里报告丢了几条）；
//   - 串口回显也由写入任务统一输出，同样不会乱行。
//
// 文件：<dir>/<baseName>.log，超过 maxFileSize 后滚动为 .1.log、.2.log ...，最多保留 maxBackups 个。
// 每行格式：2026-11-08 12:34:56.789 I [loopTask] WiFi: connected
//           （系统时间未设置时用开机时长代替：+123.456s）
//
// 用法：
//   logger.initialize();                       // 需要在 SD 卡挂载之后调用
//   logger.info("WiFi", "AP started, ip=%s", ip.c_str());
//   logger.error("SD", "open failed: %s", path);
class MdaLogger {
public:
  enum class Level : uint8_t { Debug = 0, Info, Warn, Error };

  struct LoggerConfig {
    const char *dir = "/data/logs";
    const char *baseName = "app";
    size_t maxFileSize = 1024 * 1024;  // 单个日志文件上限，默认 1MB
    uint8_t maxBackups = 5;            // 滚动保留的旧文件个数
    Level minLevel = Level::Debug;     // 低于此级别的日志直接忽略
    bool echoSerial = true;            // 同时输出到串口
    uint16_t queueLength = 64;         // 队列能缓存的行数
    uint32_t flushIntervalMs = 1000;   // 最长多久把缓冲写到卡上
  };

  static const size_t MAX_LINE_LEN = 256;  // 单行最大字节数（含换行），超出部分截断

  // 启动日志。SD 卡不可用时仍会启动（只输出串口），返回 false；之后可调用 resume() 重试。
  bool initialize(const LoggerConfig &cfg);
  bool initialize() { return initialize(LoggerConfig()); }

  void log(Level level, const char *tag, const char *fmt, ...) __attribute__((format(printf, 4, 5)));
  void debug(const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
  void info(const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
  void warn(const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
  void error(const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

  void setLevel(Level level) { _cfg.minLevel = level; }

  // 等待队列中已有的日志全部写入并落盘，最多等 timeoutMs。全部写完返回 true。
  bool flush(uint32_t timeoutMs = 2000);

  // 暂停写卡并关闭日志文件（例如格式化 SD 卡、拔卡前调用）。暂停期间的日志会在队列中等待，队列满则丢弃。
  void suspend();
  // 恢复写卡：重新创建目录、打开日志文件。成功返回 true。
  bool resume();

  bool isFileOpen() const { return _fileOpen; }
  uint32_t droppedCount() const { return _dropped.load(); }

private:
  struct LoggerItem {
    uint16_t len;
    char text[MAX_LINE_LEN];
  };

  static void taskEntry(void *arg);
  void taskLoop();
  void vlog(Level level, const char *tag, const char *fmt, va_list ap);
  void writeItem(const LoggerItem &item);
  void writeRaw(const char *text, size_t len);
  bool openFile();
  void closeFile();
  void rotateIfNeeded();
  String pathFor(int index) const;
  static bool ensureDir(const char *dir);

  LoggerConfig _cfg;
  QueueHandle_t _queue = nullptr;
  SemaphoreHandle_t _fileMutex = nullptr;  // 保护 _file：写入任务与 suspend/resume 之间互斥
  TaskHandle_t _task = nullptr;
  fs::File _file;
  volatile bool _fileOpen = false;
  volatile bool _suspended = false;
  volatile bool _busy = false;       // 写入任务正在处理一批日志
  std::atomic<uint32_t> _dropped{0};  // 队列满被丢弃的条数（未报告部分），多任务并发累加
  uint32_t _lastFlush = 0;
  bool _dirty = false;
};

extern MdaLogger logger;

#endif
