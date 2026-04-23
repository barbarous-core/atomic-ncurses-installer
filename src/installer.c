#include "installer.h"
#include <string.h>

void installer_init(installer_state_t *st)
{
    memset(st, 0, sizeof(*st));

    /* Sensible defaults */
    strncpy(st->username,    "core",              MAX_USER_LEN - 1);
    strncpy(st->keyboard,    "us",                MAX_KB_LEN   - 1);
    strncpy(st->timezone,    "UTC",               MAX_TZ_LEN   - 1);
    strncpy(st->output_path, "./barbarous.ign",   MAX_PATH_LEN - 1);
}
