#include "mda_sd_manager.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include "ff.h"

#define SD_SPI_CS_PIN   4
#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_FREQ     25000000

// SDFS 把 FatFs 的物理驱动号 _pdrv 设为 protected，这里通过成员指针读出来，
// 用来拼出 FatFs 的盘符（如 "0:"）。
struct SDPdrvAccess : fs::SDFS {
  static uint8_t get(fs::SDFS &sd) { return sd.*(&SDPdrvAccess::_pdrv); }
};



void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }

    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


void sd_setup() {
  if (!SD.begin(SD_SPI_CS_PIN)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset or reopen this serial monitor after fixing your issue!");
    while (true) {
    }
  }
  Serial.println("initialization done.");
  File root = SD.open("/");
  printDirectory(root, 0);
  root.close();
  Serial.println("done!");
}


bool sd_format() {
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  // 需要先挂载才能拿到驱动号。卡上没有文件系统时，format_if_empty=true 会先建一个。
  if (SDPdrvAccess::get(SD) == 0xFF &&
      !SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQ, "/sd", 5, true)) {
    Serial.println("SD format: card not found");
    return false;
  }

  uint8_t pdrv = SDPdrvAccess::get(SD);
  char drv[3] = {(char)('0' + pdrv), ':', 0};

  // 工作缓冲区越大格式化越快，32KB 足够
  const UINT workSize = 32 * 1024;
  BYTE *work = (BYTE *)malloc(workSize);
  if (!work) {
    Serial.println("SD format: out of memory");
    return false;
  }

  Serial.printf("SD format: formatting drive %s, please wait...\n", drv);
  uint32_t start = millis();

  // 先按 FAT32 格式化；卡太小（簇数不够）时 FatFs 会返回失败，改用自动选择
  MKFS_PARM opt = {FM_FAT32, 2, 0, 0, 0};  // 2 份 FAT 表，对齐和簇大小自动
  FRESULT res = f_mkfs(drv, &opt, work, workSize);
  if (res != FR_OK) {
    Serial.printf("SD format: FAT32 failed (%d), retry with auto type\n", res);
    opt.fmt = FM_ANY;
    res = f_mkfs(drv, &opt, work, workSize);
  }
  free(work);

  if (res != FR_OK) {
    Serial.printf("SD format: f_mkfs failed (%d)\n", res);
    return false;
  }
  Serial.printf("SD format: done in %lu ms\n", (unsigned long)(millis() - start));

  // 格式化后旧的挂载信息已失效，重新挂载
  SD.end();
  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQ)) {
    Serial.println("SD format: remount failed");
    return false;
  }
  Serial.printf("SD format: remounted, %.2f GB free\n",
                (double)(SD.totalBytes() - SD.usedBytes()) / 1024 / 1024 / 1024);
  return true;
}

bool sd_is_mounted() {
  return SDPdrvAccess::get(SD) != 0xFF;
}

bool sd_mount() {
  if (sd_is_mounted()) {
    return true;
  }
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQ)) {
    Serial.println("SD: mount failed (没插卡或格式不是 FAT32?)");
    return false;
  }
  Serial.printf("SD: mounted, %.2f GB total\n", (double)SD.totalBytes() / 1024 / 1024 / 1024);
  return true;
}

bool sd_init_dirs() {
  if (!sd_is_mounted()) {
    return false;
  }
  // 按层级顺序创建，父目录在前
  static const char *const dirs[] = {MDA_DATA_DIR, MDA_CONFIG_DIR, MDA_LOG_DIR};
  bool ok = true;
  for (const char *dir : dirs) {
    if (SD.exists(dir)) {
      continue;
    }
    if (SD.mkdir(dir)) {
      Serial.printf("SD: created %s\n", dir);
    } else {
      Serial.printf("SD: mkdir %s failed\n", dir);
      ok = false;
    }
  }
  return ok;
}
