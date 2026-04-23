#ifndef INSTALLER_H
#define INSTALLER_H

#include <stdbool.h>

/* ─── Screen navigation return codes ─────────────────────────────────────── */
#define NAV_NEXT   1
#define NAV_PREV  -1
#define NAV_QUIT   0

/* ─── Max sizes ───────────────────────────────────────────────────────────── */
#define MAX_DISK_LEN     64
#define MAX_USER_LEN     64
#define MAX_HASH_LEN    256
#define MAX_TZ_LEN       64
#define MAX_KB_LEN       32
#define MAX_PATH_LEN    256
#define MAX_RPMS        128
#define MAX_RPM_LEN     256
#define MAX_BINS        128
#define MAX_BIN_LEN     256
#define MAX_DOTFILES    128
#define MAX_DOTFILE_LEN 256

/* ─── Installer state (passed through every screen) ──────────────────────── */
typedef struct {
    /* Step 1 – Disk */
    char disk[MAX_DISK_LEN];          /* e.g. /dev/sda            */

    /* Step 2 – User */
    char username[MAX_USER_LEN];
    char password_hash[MAX_HASH_LEN]; /* openssl-generated sha512 */

    /* Step 3 – Locale */
    char keyboard[MAX_KB_LEN];        /* e.g. us, fr, de          */
    char timezone[MAX_TZ_LEN];        /* e.g. America/New_York    */

    /* Step 4 – RPM packages */
    char rpms[MAX_RPMS][MAX_RPM_LEN];
    int  rpm_count;

    /* Step 5 – Bins to inject */
    char bins[MAX_BINS][MAX_BIN_LEN]; /* relative paths from ISO  */
    int  bin_count;

    /* Step 6 – Dotfiles */
    char dotfiles[MAX_DOTFILES][MAX_DOTFILE_LEN];
    int  dotfile_count;

    /* Output */
    char output_path[MAX_PATH_LEN];   /* where .ign is written    */
} installer_state_t;

/* Initialise state with sane defaults */
void installer_init(installer_state_t *st);

#endif /* INSTALLER_H */
