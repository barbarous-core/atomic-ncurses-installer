#include "scr_setup_paths.h"
#include "../common/ui.h"
#include <string.h>

static void draw_paths(setup_state_t *st, int field)
{
    ui_clear_body();
    int bh = ui_body_height();
    int bw = ui_body_width();
    int top = ui_body_top();

    int boxw = bw - 8;
    int boxh = 14;
    int boxy = top + (bh - boxh) / 2;
    int boxx = 4;

    WINDOW *win = newwin(boxh, boxw, boxy, boxx);
    wbkgd(win, COLOR_PAIR(CP_NORMAL));
    ui_box(win, CP_BORDER);

    ui_center(win, 1, "Path Configuration", CP_ACCENT, A_BOLD);

    int ly = 3;
    int lx = 4;
    int iw = boxw - 30;

    mvwprintw(win, ly, lx, "ISO Device/Path:");
    ui_button(win, ly, lx + 20, st->iso_path, field == 0);

    ly += 2;
    mvwprintw(win, ly, lx, "Bin Dest Path:");
    ui_button(win, ly, lx + 20, st->bin_dest_path, field == 1);

    ly += 2;
    mvwprintw(win, ly, lx, "RPM Dest Path:");
    ui_button(win, ly, lx + 20, st->rpm_dest_path, field == 2);

    ly += 2;
    mvwprintw(win, ly, lx, "Dotfiles Dest:");
    ui_button(win, ly, lx + 20, st->dotfiles_dest_path, field == 3);

    ui_button(win, boxh - 2, (boxw / 2) - 10, "  CONTINUE  ", field == 4);

    wrefresh(win);
    delwin(win);
}

int screen_setup_paths(setup_state_t *st)
{
    int field = 0;
    const char *footer[] = {
        "TAB/Arrows Switch",
        "ENTER Edit/Next",
        "ESC Back",
    };

    while (1) {
        ui_draw_header("Step 1 of 3  —  Asset Paths");
        ui_draw_footer(footer, 3);
        draw_paths(st, field);

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                field = (field + 4) % 5;
                break;
            case KEY_DOWN: case '\t':
                field = (field + 1) % 5;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case '\n': case KEY_ENTER:
                if (field == 4) return NAV_NEXT;
                
                /* Edit field */
                {
                    int bh = ui_body_height();
                    int bw = ui_body_width();
                    int top = ui_body_top();
                    int boxh = 14;
                    int boxy = top + (bh - boxh) / 2;
                    int boxx = 4;
                    int ly = 3 + (field * 2);
                    
                    WINDOW *edit = newwin(1, bw - 40, boxy + ly, boxx + 24);
                    wbkgd(edit, COLOR_PAIR(CP_SELECTED));
                    curs_set(1);
                    char *buf = NULL;
                    if (field == 0) buf = st->iso_path;
                    if (field == 1) buf = st->bin_dest_path;
                    if (field == 2) buf = st->rpm_dest_path;
                    if (field == 3) buf = st->dotfiles_dest_path;
                    
                    ui_readline(edit, 0, 0, bw - 40, buf, MAX_PATH_LEN, false);
                    curs_set(0);
                    delwin(edit);
                }
                break;
            case 27:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case KEY_RESIZE:
                break;
        }
    }
}
