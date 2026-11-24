#include <mda_battery.h>

#ifndef MDA_SIM
#include <M5Unified.h>
bool mda_is_external_battery() {
    return M5.Power.getVBUSVoltage() > 4000 && !M5.Power.getUsbOutput();
}

bool mda_is_charging() {
    return M5.Power.isCharging() == m5::Power_Class::is_charging;
}

int mda_battery_level() {
    return M5.Power.getBatteryLevel();
}
#else
bool mda_is_external_battery() {
    return true;
}
bool mda_is_charging() {
    return false;
}

int mda_battery_level() {
    return 100;
}
#endif
