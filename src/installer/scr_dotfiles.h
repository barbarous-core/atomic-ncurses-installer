#ifndef SCREEN_DOTFILES_H
#define SCREEN_DOTFILES_H

#include "installer.h"

/* Displays a list of dotfiles available in the assets directory.
 * Populates st->dotfiles and st->dotfile_count based on user selection.
 * Returns NAV_NEXT, NAV_PREV, or NAV_QUIT. */
int screen_dotfiles(installer_state_t *st);

#endif /* SCREEN_DOTFILES_H */
