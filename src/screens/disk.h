#ifndef SCREEN_DISK_H
#define SCREEN_DISK_H

#include "../installer.h"

/* Displays an interactive block-device list.
 * Populates st->disk (e.g. "/dev/sda") on selection.
 * Returns NAV_NEXT, NAV_PREV, or NAV_QUIT. */
int screen_disk(installer_state_t *st);

#endif /* SCREEN_DISK_H */
