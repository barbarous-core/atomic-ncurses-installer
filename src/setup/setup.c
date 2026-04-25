#include "setup.h"
#include <string.h>
#include <glob.h>

void setup_init(setup_state_t *st)
{
    memset(st, 0, sizeof(setup_state_t));
    
    /* Sane defaults */
    strncpy(st->iso_path, "/dev/sr0", MAX_PATH_LEN - 1);
    strncpy(st->bin_dest_path, "/run/media/iso/barbarous-assets/bin", MAX_PATH_LEN - 1);
    strncpy(st->rpm_dest_path, "/run/media/iso/barbarous-assets/rpms", MAX_PATH_LEN - 1);
    strncpy(st->dotfiles_dest_path, "/run/media/iso/barbarous-assets/dotfiles", MAX_PATH_LEN - 1);
    /* Search for the generated matrix file in user's home */
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_t));
    
    int return_value = glob("/home/*/.barbarous/matrix_*.csv", 0, NULL, &glob_result);
    if(return_value == 0 && glob_result.gl_pathc > 0){
        strncpy(st->matrix_path, glob_result.gl_pathv[0], MAX_PATH_LEN - 1);
    } else {
        strncpy(st->matrix_path, "/run/media/iso/barbarous-assets/assets/matrix.csv", MAX_PATH_LEN - 1);
    }
    globfree(&glob_result);
}
