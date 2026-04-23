#ifndef SCREEN_LOCALE_H
#define SCREEN_LOCALE_H

#include "installer.h"

/* Displays the locale configuration screen (keyboard & timezone).
 * Populates st->keyboard and st->timezone on success.
 * Returns NAV_NEXT, NAV_PREV, or NAV_QUIT. */
int screen_locale(installer_state_t *st);

#endif /* SCREEN_LOCALE_H */
