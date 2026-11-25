#ifndef MDA_STATUSBAR_H
#define MDA_STATUSBAR_H

#include <lvgl.h>
#include "ui/mda_ui.h"

lv_obj_t *mda_statusbar_create(const mda_pages_t *pages);
void mda_statusbar_notify(int32_t value);
void mda_statusbar_notify(int32_t value, const char *title);

#endif
