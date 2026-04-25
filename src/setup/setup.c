#include "setup.h"
#include <string.h>

void setup_init(setup_state_t *st)
{
    memset(st, 0, sizeof(setup_state_t));
    
    /* Sane defaults */
    strncpy(st->iso_path, "/dev/sr0", MAX_PATH_LEN - 1);
    strncpy(st->bin_dest_path, "/run/media/iso/barbarous-assets/bin", MAX_PATH_LEN - 1);
    strncpy(st->rpm_dest_path, "/run/media/iso/barbarous-assets/rpms", MAX_PATH_LEN - 1);
    strncpy(st->dotfiles_dest_path, "/run/media/iso/barbarous-assets/dotfiles", MAX_PATH_LEN - 1);
    strncpy(st->matrix_path, "/run/media/iso/barbarous-assets/matrix.csv", MAX_PATH_LEN - 1);
}
