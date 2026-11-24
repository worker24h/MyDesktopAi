#ifndef MDA_STATUSBAR_H
#define MDA_STATUSBAR_H

#include <lvgl.h>
lv_obj_t *mda_statusbar_create();
void mda_statusbar_notify(int32_t value);
#endif
