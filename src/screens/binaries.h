#ifndef SCREEN_BINARIES_H
#define SCREEN_BINARIES_H

#include "../installer.h"

/* Displays a list of executable binaries available in the assets directory.
 * Populates st->bins and st->bin_count based on user selection.
 * Returns NAV_NEXT, NAV_PREV, or NAV_QUIT. */
int screen_binaries(installer_state_t *st);

#endif /* SCREEN_BINARIES_H */
