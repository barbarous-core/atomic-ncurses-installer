#include "scr_setup_matrix.h"
#include "../common/ui.h"
#include <string.h>
#include <dirent.h>
#include <glob.h>

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

    char preconf_buf[256];
    if (strlen(st->matrix_path) > 0) {
        snprintf(preconf_buf, sizeof(preconf_buf), "Configured Path (%s)", st->matrix_path);
        menu_items[item_count++] = preconf_buf;
    }

    for (int i = 0; i < local_count; i++) {
        menu_items[item_count++] = local_files[i];
    }
    menu_items[item_count++] = "Load from home directory (~/.barbarous/matrix_[user]_[date].csv)";
    menu_items[item_count++] = "Load from ISO (/run/media/iso/barbarous-assets/assets/matrix.csv)";
    menu_items[item_count++] = "Load from URL (Custom)";

    ui_draw_header("Step 2 of 4  —  Matrix Configuration");
    int sel = ui_menu("Select Matrix Source", menu_items, item_count, 0);

    if (sel == -1) return NAV_PREV;

    int current_idx = 0;
    if (strlen(st->matrix_path) > 0) {
        if (sel == current_idx) {
            // Keep st->matrix_path as is
            ui_msgbox_timed("Matrix Configured", "Using the preconfigured matrix path.", CP_SUCCESS, 1);
            return NAV_NEXT;
        }
        current_idx++;
    }

    if (sel < current_idx + local_count) {
        strncpy(st->matrix_path, local_files[sel - current_idx], MAX_PATH_LEN - 1);
    } else if (sel == current_idx + local_count) {
        /* Load from home directory */
        glob_t glob_result;
        memset(&glob_result, 0, sizeof(glob_t));
        int return_value = glob("/home/*/.barbarous/matrix_*.csv", 0, NULL, &glob_result);
        if(return_value == 0 && glob_result.gl_pathc > 0){
            strncpy(st->matrix_path, glob_result.gl_pathv[0], MAX_PATH_LEN - 1);
            ui_msgbox_timed("Matrix Configured", "Loaded from home directory.", CP_SUCCESS, 1);
        } else {
            ui_msgbox("Error", "No matrix file found in ~/.barbarous/", CP_DANGER);
            globfree(&glob_result);
            return NAV_PREV;
        }
        globfree(&glob_result);
        return NAV_NEXT;
    } else if (sel == current_idx + local_count + 1) {
        strncpy(st->matrix_path, "/run/media/iso/barbarous-assets/assets/matrix.csv", MAX_PATH_LEN - 1);
    } else {
        /* URL Selection */
        ui_clear_body();
        ui_draw_header("Step 2 of 4  —  Matrix URL");
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
