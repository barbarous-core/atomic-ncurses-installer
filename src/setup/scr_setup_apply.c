#include "scr_setup_apply.h"
#include "ui.h"
#include <unistd.h>

static void draw_progress(setup_state_t *st, const char *msg, int percent)
{
    ui_clear_body();
    int bh = ui_body_height();
    int bw = ui_body_width();
    int top = ui_body_top();

    WINDOW *win = newwin(12, bw - 20, top + (bh - 12) / 2, 10);
    wbkgd(win, COLOR_PAIR(CP_NORMAL));
    ui_box(win, CP_BORDER);

    ui_center(win, 1, "Applying Configuration", CP_ACCENT, A_BOLD);
    
    mvwprintw(win, 3, 4, "ISO: %s", st->iso_path);
    mvwprintw(win, 4, 4, "Matrix: %s", st->matrix_path);

    ui_center(win, 6, msg, CP_NORMAL, A_NORMAL);
    ui_progress_bar(win, 8, 4, bw - 28, percent, CP_SUCCESS);

    wrefresh(win);
    delwin(win);
}

int screen_setup_apply(setup_state_t *st)
{
    ui_draw_header("Applying Setup");
    
    if (!ui_confirm("Confirm Setup", "Are you sure you want to apply the system setup? This will modify your system layers and files.")) {
        return NAV_PREV;
    }

    /* 1. RPM Layer System */
    for (int i = 0; i <= 33; i++) {
        draw_progress(st, "Installing RPM layer system...", i);
        usleep(30000);
    }

    /* 2. Copy Binaries */
    for (int i = 34; i <= 66; i++) {
        draw_progress(st, "Copying binaries to system paths...", i);
        usleep(20000);
    }

    /* 3. Copy Dotfiles */
    for (int i = 67; i <= 100; i++) {
        draw_progress(st, "Synchronizing dotfiles...", i);
        usleep(20000);
    }

    ui_msgbox("Setup Complete", "Barbarous system setup has been completed successfully.\n\nYou may need to restart your shell or system for changes to take effect.", CP_SUCCESS);

    return NAV_QUIT;
}
