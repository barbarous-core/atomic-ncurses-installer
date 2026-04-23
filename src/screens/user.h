#ifndef SCREEN_USER_H
#define SCREEN_USER_H

#include "installer.h"

/* Displays the user account and password screen.
 * Populates st->username and st->password_hash on success.
 * Returns NAV_NEXT, NAV_PREV, or NAV_QUIT. */
int screen_user(installer_state_t *st);

#endif /* SCREEN_USER_H */
