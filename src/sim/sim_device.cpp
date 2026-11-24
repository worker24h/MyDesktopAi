#include "hal/mda_device.h"
#include "mda_logger.h"

void mda_device_set_volume(uint8_t percent) {
    logger.info("sim", "Speaker volume set to %d%%", percent);
}

void mda_device_set_brightness(uint8_t percent) {
    logger.info("sim", "Display brightness set to %d%%", percent);
}
