#include "mda_logger.h"

#include <SD.h>
#include <sys/time.h>
#include <time.h>

MdaLogger logger;

static const char LEVEL_CHARS[] = {'D', 'I', 'W', 'E'};


bool MdaLogger::initialize(const LoggerConfig &cfg) {
  if (_task) {
    return _fileOpen;  // 已经启动过
  }
  _cfg = cfg;

  _queue = xQueueCreate(_cfg.queueLength, sizeof(LoggerItem));
  _fileMutex = xSemaphoreCreateMutex();
  if (!_queue || !_fileMutex) {
    Serial.println("[logger] create queue/mutex failed");
    return false;
  }

  xSemaphoreTake(_fileMutex, portMAX_DELAY);
  bool ok = openFile();
  xSemaphoreGive(_fileMutex);

  // 写入任务优先级低于 loopTask(1)，SD 卡写入不会抢占业务逻辑
  xTaskCreate(taskEntry, "sdlog", 4096, this, tskIDLE_PRIORITY + 1, &_task);

  info("logger", "started, file=%s, sd=%s", pathFor(0).c_str(), ok ? "ok" : "unavailable");
  return ok;
}

void MdaLogger::log(Level level, const char *tag, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(level, tag, fmt, ap);
  va_end(ap);
}

void MdaLogger::debug(const char *tag, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(Level::Debug, tag, fmt, ap);
  va_end(ap);
}

void MdaLogger::info(const char *tag, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(Level::Info, tag, fmt, ap);
  va_end(ap);
}

void MdaLogger::warn(const char *tag, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(Level::Warn, tag, fmt, ap);
  va_end(ap);
}

void MdaLogger::error(const char *tag, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vlog(Level::Error, tag, fmt, ap);
  va_end(ap);
}

bool MdaLogger::flush(uint32_t timeoutMs) {
  if (!_queue) {
    return true;
  }
  uint32_t start = millis();
  // 等队列清空且写入任务处理完当前批次（写入任务在批次结束时会落盘）
  while (uxQueueMessagesWaiting(_queue) > 0 || _busy) {
    if (millis() - start >= timeoutMs) {
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  xSemaphoreTake(_fileMutex, portMAX_DELAY);
  if (_fileOpen) {
    _file.flush();
    _dirty = false;
  }
  xSemaphoreGive(_fileMutex);
  return true;
}

void MdaLogger::suspend() {
  if (!_fileMutex) {
    return;
  }
  flush();
  xSemaphoreTake(_fileMutex, portMAX_DELAY);
  _suspended = true;
  closeFile();
  xSemaphoreGive(_fileMutex);
  Serial.println("[logger] suspended");
}

bool MdaLogger::resume() {
  if (!_fileMutex) {
    return false;
  }
  xSemaphoreTake(_fileMutex, portMAX_DELAY);
  bool ok = openFile();
  _suspended = false;
  xSemaphoreGive(_fileMutex);
  info("logger", "resumed, sd=%s", ok ? "ok" : "unavailable");
  return ok;
}

// ---------------- 生产者：在调用者任务中执行 ----------------

void MdaLogger::vlog(Level level, const char *tag, const char *fmt, va_list ap) {
  if (!_queue || level < _cfg.minLevel) {
    return;
  }

  LoggerItem item;
  size_t n = 0;
  const size_t cap = MAX_LINE_LEN - 1;  // 末尾留 1 字节给 '\n'

  // 时间戳：系统时间有效（已对时）用日期时间，否则用开机时长
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  if (tv.tv_sec > 1700000000) {
    struct tm tmv;
    localtime_r(&tv.tv_sec, &tmv);
    n = strftime(item.text, cap, "%Y-%m-%d %H:%M:%S", &tmv);
    n += snprintf(item.text + n, cap - n, ".%03ld", (long)(tv.tv_usec / 1000));
  } else {
    uint32_t ms = millis();
    n = snprintf(item.text, cap, "+%lu.%03lus", (unsigned long)(ms / 1000), (unsigned long)(ms % 1000));
  }
  if (n >= cap) {
    n = cap - 1;
  }

  const char *taskName = pcTaskGetName(nullptr);
  int w = snprintf(item.text + n, cap - n, " %c [%s] %s: ", LEVEL_CHARS[(uint8_t)level],
                   taskName ? taskName : "?", tag ? tag : "-");
  if (w > 0) {
    n += ((size_t)w < cap - n) ? (size_t)w : cap - n - 1;
  }

  w = vsnprintf(item.text + n, cap - n, fmt, ap);
  if (w > 0) {
    n += ((size_t)w < cap - n) ? (size_t)w : cap - n - 1;
  }

  // 一条日志严格对应一行：把消息里的换行替换成空格，再统一补一个 '\n'
  for (size_t i = 0; i < n; i++) {
    if (item.text[i] == '\n' || item.text[i] == '\r') {
      item.text[i] = ' ';
    }
  }
  item.text[n++] = '\n';
  item.len = (uint16_t)n;

  // 不阻塞调用者：队列满就丢弃并计数
  if (xQueueSend(_queue, &item, 0) != pdTRUE) {
    _dropped.fetch_add(1);
  }
}

// ---------------- 消费者：唯一的写入任务 ----------------

void MdaLogger::taskEntry(void *arg) {
  static_cast<MdaLogger *>(arg)->taskLoop();
}

void MdaLogger::taskLoop() {
  LoggerItem item;
  for (;;) {
    // 暂停期间不取队列，日志留在队列里等恢复
    if (_suspended) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    if (xQueueReceive(_queue, &item, pdMS_TO_TICKS(_cfg.flushIntervalMs)) == pdTRUE) {
      _busy = true;
      xSemaphoreTake(_fileMutex, portMAX_DELAY);

      // 先报告之前因队列满丢掉的日志条数
      uint32_t dropped = _dropped.exchange(0);
      if (dropped > 0) {
        char note[64];
        int len = snprintf(note, sizeof(note), "+%lus W [sdlog] logger: dropped %lu messages (queue full)\n",
                           (unsigned long)(millis() / 1000), (unsigned long)dropped);
        writeRaw(note, len);
      }

      // 写当前这条，再把队列里已有的一次性写完（批量写，减少 SD 卡开销）
      writeItem(item);
      while (!_suspended && xQueueReceive(_queue, &item, 0) == pdTRUE) {
        writeItem(item);
      }

      rotateIfNeeded();
      xSemaphoreGive(_fileMutex);
      _busy = false;
    }

    // 定时落盘：断电时最多丢失 flushIntervalMs 内的日志
    if (_dirty && millis() - _lastFlush >= _cfg.flushIntervalMs) {
      xSemaphoreTake(_fileMutex, portMAX_DELAY);
      if (_fileOpen) {
        _file.flush();
      }
      _dirty = false;
      _lastFlush = millis();
      xSemaphoreGive(_fileMutex);
    }
  }
}

// 以下函数都在持有 _fileMutex 时调用

void MdaLogger::writeItem(const LoggerItem &item) {
  writeRaw(item.text, item.len);
}

void MdaLogger::writeRaw(const char *text, size_t len) {
  if (_cfg.echoSerial) {
    Serial.write((const uint8_t *)text, len);
  }
  if (_fileOpen) {
    if (_file.write((const uint8_t *)text, len) != len) {
      // 写失败（卡被拔出等）：关闭文件，之后只输出串口，可调用 resume() 恢复
      Serial.println("[logger] write failed, file closed");
      closeFile();
      return;
    }
    _dirty = true;
  }
}

bool MdaLogger::openFile() {
  closeFile();
  if (!ensureDir(_cfg.dir)) {
    return false;
  }
  _file = SD.open(pathFor(0), FILE_APPEND);
  _fileOpen = (bool)_file;
  _dirty = false;
  _lastFlush = millis();
  return _fileOpen;
}

void MdaLogger::closeFile() {
  if (_fileOpen) {
    _file.flush();
    _file.close();
  }
  _fileOpen = false;
  _dirty = false;
}

/**
 * 滚动保存日志
 */
void MdaLogger::rotateIfNeeded() {
  // 用 position() 而不是 size()：追加模式下位置就是文件末尾，且不受未落盘缓存影响
  if (!_fileOpen || _file.position() < _cfg.maxFileSize) {
    return;
  }

  closeFile();
  // app.(N-1).log -> app.N.log ... app.log -> app.1.log，最旧的删除
  if (_cfg.maxBackups > 0) {
    String oldest = pathFor(_cfg.maxBackups);
    if (SD.exists(oldest)) {
      SD.remove(oldest);
    }
    
    for (int i = _cfg.maxBackups - 1; i >= 0; i--) {
      String from = pathFor(i);
      if (SD.exists(from)) {
        SD.rename(from, pathFor(i + 1));
      }
    }
  } else {
    SD.remove(pathFor(0));
  }
  openFile();
}

String MdaLogger::pathFor(int index) const {
  String p = String(_cfg.dir) + "/" + _cfg.baseName;
  if (index > 0) {
    p += "." + String(index);
  }
  return p + ".log";
}

/**
 * 逐级创建目录，如 /data/logs 会依次创建 /data、/data/logs
 */
bool MdaLogger::ensureDir(const char *dir) {
  String path(dir);
  if (path.length() == 0 || path == "/") {
    return true;
  }
  
  int pos = 1;
  while (true) {
    int next = path.indexOf('/', pos);
    String part = (next < 0) ? path : path.substring(0, next);
    if (!SD.exists(part) && !SD.mkdir(part)) {
      Serial.printf("[logger] mkdir %s failed\n", part.c_str());
      return false;
    }
    if (next < 0) {
      break;
    }
    pos = next + 1;
  }
  return true;
}
