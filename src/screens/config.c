#include "config.h"
#include "../ui.h"
#include "../installer.h"
#include <ncurses.h>
#include <string.h>

#define FLD_HOSTNAME 0
#define FLD_SSH_KEY  1
#define BTN_BACK     2
#define BTN_NEXT     3
#define NUM_FOCUS    4

static void draw_config_screen(const installer_state_t *st, int focus, const char *err_msg)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    int row = top + 2;
    ui_center(stdscr, row, "System Configuration", CP_NORMAL, A_BOLD);
    row += 2;
    ui_center(stdscr, row, "Set your hostname and provide an SSH public key for access.", CP_DIM, A_NORMAL);
    row += 3;

    int box_w = 60;
    int box_h = 10;
    int box_x = (bw - box_w) / 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    mvwaddstr(box, 2, 4, "Hostname:");
    mvwaddstr(box, 4, 4, "SSH Key: ");

    /* Hostname field */
    if (focus == FLD_HOSTNAME) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 2, 14, st->hostname[0] ? st->hostname : "(empty)", 40);
    if (focus == FLD_HOSTNAME) wattroff(box, COLOR_PAIR(CP_SELECTED));

    /* SSH Key field (shortened display) */
    if (focus == FLD_SSH_KEY) wattron(box, COLOR_PAIR(CP_SELECTED));
    if (st->ssh_key[0]) {
        char short_key[41];
        strncpy(short_key, st->ssh_key, 37);
        short_key[37] = '.'; short_key[38] = '.'; short_key[39] = '.'; short_key[40] = '\0';
        mvwaddstr(box, 4, 14, short_key);
    } else {
        mvwaddstr(box, 4, 14, "(paste your public key here)");
    }
    if (focus == FLD_SSH_KEY) wattroff(box, COLOR_PAIR(CP_SELECTED));

    wrefresh(box);
    delwin(box);

    row += box_h + 1;
    if (err_msg[0]) {
        ui_center(stdscr, row, err_msg, CP_DANGER, A_BOLD);
    }

    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

int screen_config(installer_state_t *st)
{
    int focus = FLD_HOSTNAME;
    char err_msg[128] = {0};
    
    const char *footer[] = {
        "TAB  Navigate",
        "ENTER Edit/Select",
        "← Back",
        "Q Quit",
    };

    while (1) {
        ui_draw_header("Step 2 of 4  —  System Configuration");
        ui_draw_footer(footer, 4);
        draw_config_screen(st, focus, err_msg);

        int ch = getch();
        err_msg[0] = '\0';

        switch (ch) {
            case KEY_UP: case 'k':
                focus = (focus - 1 + NUM_FOCUS) % NUM_FOCUS;
                break;
            case KEY_DOWN: case 'j': case '\t':
                focus = (focus + 1) % NUM_FOCUS;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (focus == FLD_HOSTNAME) {
                    WINDOW *box = newwin(10, 60, ui_body_top() + 7, (ui_body_width() - 60) / 2);
                    ui_readline(box, 2, 14, 40, st->hostname, MAX_HOSTNAME_LEN, false);
                    delwin(box);
                } else if (focus == FLD_SSH_KEY) {
                    WINDOW *box = newwin(10, 60, ui_body_top() + 7, (ui_body_width() - 60) / 2);
                    ui_readline(box, 4, 14, 40, st->ssh_key, MAX_SSH_LEN, false);
                    delwin(box);
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
                    if (st->hostname[0] == '\0') {
                        strncpy(err_msg, "Hostname cannot be empty.", sizeof(err_msg)-1);
                    } else if (st->ssh_key[0] == '\0') {
                        strncpy(err_msg, "SSH key is required for initial access.", sizeof(err_msg)-1);
                    } else {
                        return NAV_NEXT;
                    }
                }
                break;
            case KEY_RESIZE:
                break;
            default:
                break;
        }
    }
}
