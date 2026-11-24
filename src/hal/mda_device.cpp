#include "mda_device.h"
#include <M5Unified.h>

void mda_device_set_volume(uint8_t percent) {
    // 加 50 再除以 100 是四舍五入；直接除会向下取整，比如 50 会得到 127 而不是 128
    uint8_t value = (uint8_t)((percent * 255 + 50) / 100);
    M5.Speaker.setVolume(value);
}

void mda_device_set_brightness(uint8_t percent) {
    uint8_t value = (uint8_t)((percent * 255 + 50) / 100);
    if (value < 10 && percent > 0) {
        value = 10; // 下限保护
    }
    M5.Display.setBrightness(value);
}
