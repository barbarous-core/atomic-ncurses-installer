#ifndef SCREEN_WELCOME_H
#define SCREEN_WELCOME_H

#include "installer.h"

/* Displays the welcome / splash screen.
 * Returns NAV_NEXT to proceed, NAV_QUIT to abort. */
int screen_welcome(installer_state_t *st);

#endif /* SCREEN_WELCOME_H */
