#include "installer.h"
#include <string.h>
#include <stdio.h>

void installer_init(installer_state_t *st)
{
    memset(st, 0, sizeof(*st));

    /* Sensible defaults */
    strncpy(st->edition,     "Core",              MAX_EDITION_LEN - 1);
    strncpy(st->hostname,    "barbarous",         MAX_HOSTNAME_LEN - 1);
    strncpy(st->username,    "core",              MAX_USER_LEN - 1);
    strncpy(st->keyboard,    "us",                MAX_KB_LEN   - 1);
    strncpy(st->timezone,    "UTC",               MAX_TZ_LEN   - 1);
    st->install_dotfiles = true;
    snprintf(st->output_path, MAX_PATH_LEN, "barbarous.ign");
}
