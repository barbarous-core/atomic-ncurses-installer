#ifndef SETUP_H
#define SETUP_H

#include <stdbool.h>

/* ─── Screen navigation return codes ─────────────────────────────────────── */
#define NAV_NEXT   1
#define NAV_PREV  -1
#define NAV_QUIT   0

/* ─── Max sizes ───────────────────────────────────────────────────────────── */
#define MAX_PATH_LEN    256
#define MAX_URL_LEN     512
#define MAX_RPM_LEN     256
#define MAX_RPMS        512
#define MAX_BIN_LEN     256
#define MAX_BINS        512
#define MAX_DOTFILE_LEN 256
#define MAX_DOTFILES    128

/* ─── Setup state ────────────────────────────────────────────────────────── */
typedef struct {
    char iso_path[MAX_PATH_LEN];
    char bin_dest_path[MAX_PATH_LEN];
    char rpm_dest_path[MAX_PATH_LEN];
    char dotfiles_dest_path[MAX_PATH_LEN];

    char matrix_path[MAX_PATH_LEN];
    char matrix_url[MAX_URL_LEN];
    char edition[64];

    /* Data from matrix.csv */
    char rpms[MAX_RPMS][MAX_RPM_LEN];
    int  rpm_count;

    char bins[MAX_BINS][MAX_BIN_LEN];
    int  bin_count;

    char dotfiles[MAX_DOTFILES][MAX_DOTFILE_LEN];
    int  dotfile_count;

} setup_state_t;

void setup_init(setup_state_t *st);

#endif /* SETUP_H */
