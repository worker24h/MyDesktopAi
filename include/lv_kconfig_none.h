/*
 * 空文件，故意留空。
 * LVGL 9.2 在 ESP32 上编译汇编文件 (.S) 时，lv_conf_kconfig.h 会尝试包含 esp_attr.h 而报错。
 * platformio.ini 里用 LV_CONF_KCONFIG_EXTERNAL_INCLUDE 指向这个空文件，跳过那段 Kconfig 逻辑。
 * 本工程用 include/lv_conf.h 配置 LVGL，不使用 Kconfig，所以不受影响。
 */
