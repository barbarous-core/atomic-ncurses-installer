#ifndef SCREEN_PACKAGES_H
#define SCREEN_PACKAGES_H

#include "installer.h"

/* Displays a list of RPM packages available in the assets directory.
 * Populates st->rpms and st->rpm_count based on user selection.
 * Returns NAV_NEXT, NAV_PREV, or NAV_QUIT. */
int screen_packages(installer_state_t *st);

#endif /* SCREEN_PACKAGES_H */
