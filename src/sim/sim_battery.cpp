#include "mda_battery.h"

bool mda_is_external_battery() {
    return true;
}

bool mda_is_charging() {
    return false;
}

int mda_battery_level() {
    return 100;
}
