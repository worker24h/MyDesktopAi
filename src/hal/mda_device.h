#ifndef MDA_DEVICE_H
#define MDA_DEVICE_H

#include <stdint.h>

// 设置扬声器音量（0 ~ 100）
void mda_device_set_volume(uint8_t percent);

// 设置屏幕亮度（0 ~ 100）
void mda_device_set_brightness(uint8_t percent);

#endif
