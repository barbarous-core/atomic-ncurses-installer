#ifndef SCREEN_GENERATE_H
#define SCREEN_GENERATE_H

#include "installer.h"

/* Displays a summary of the configuration and generates the Ignition file.
 * Returns NAV_QUIT when done (or NAV_PREV to go back). */
int screen_generate(installer_state_t *st);

#endif /* SCREEN_GENERATE_H */
