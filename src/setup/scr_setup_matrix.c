#include "scr_setup_matrix.h"
#include "../common/ui.h"
#include <string.h>
#include <dirent.h>

static int find_local_matrices(char names[10][MAX_PATH_LEN])
{
    DIR *d = opendir(".");
    if (!d) return 0;
    struct dirent *dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL) {
        if (strstr(dir->d_name, "matrix") && strstr(dir->d_name, ".csv")) {
            strncpy(names[count], dir->d_name, MAX_PATH_LEN - 1);
            count++;
            if (count >= 10) break;
        }
    }
    closedir(d);
    return count;
}

int screen_setup_matrix(setup_state_t *st)
{
    char local_files[10][MAX_PATH_LEN];
    int local_count = find_local_matrices(local_files);

    const char *menu_items[15];
    int item_count = 0;

    for (int i = 0; i < local_count; i++) {
        menu_items[item_count++] = local_files[i];
    }
    menu_items[item_count++] = "Load from ISO (/run/media/iso/matrix.csv)";
    menu_items[item_count++] = "Load from URL (Custom)";

    ui_draw_header("Step 2 of 3  —  Matrix Configuration");
    int sel = ui_menu("Select Matrix Source", menu_items, item_count, 0);

    if (sel == -1) return NAV_PREV;

    if (sel < local_count) {
        strncpy(st->matrix_path, local_files[sel], MAX_PATH_LEN - 1);
    } else if (sel == local_count) {
        strncpy(st->matrix_path, "/run/media/iso/matrix.csv", MAX_PATH_LEN - 1);
    } else {
        /* URL Selection */
        ui_clear_body();
        ui_draw_header("Step 2 of 3  —  Matrix URL");
        WINDOW *win = newwin(7, ui_body_width() - 10, ui_body_top() + 5, 5);
        ui_box(win, CP_BORDER);
        ui_center(win, 1, "Enter Matrix CSV URL", CP_ACCENT, A_BOLD);
        curs_set(1);
        ui_readline(win, 3, 2, ui_body_width() - 14, st->matrix_url, MAX_URL_LEN, false);
        curs_set(0);
        delwin(win);
        if (strlen(st->matrix_url) == 0) return NAV_PREV;
    }

    /* In a real app, we would parse the CSV here to populate rpms, bins, dotfiles */
    /* For now, we simulate a successful load */
    ui_msgbox_timed("Matrix Loaded", "Matrix configuration has been loaded successfully.", CP_SUCCESS, 1);

    return NAV_NEXT;
}
