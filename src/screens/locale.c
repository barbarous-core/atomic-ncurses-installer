#include "locale.h"
#include "../ui.h"
#include "../installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define NUM_FIELDS 4
#define FLD_KB     0
#define FLD_TZ     1
#define BTN_BACK   2
#define BTN_NEXT   3

static void draw_locale_screen(const installer_state_t *st, int focus, const char *err_msg)
{
    ui_clear_body();

    int bw  = ui_body_width();
    int top = ui_body_top();

    /* ── Instructions ── */
    int row = top + 2;
    ui_center(stdscr, row, "Configure the system locale and timezone.", CP_NORMAL, A_BOLD);
    row += 2;

    /* ── Form box ── */
    int box_w = 60;
    int box_h = 8;
    int box_x = (bw - box_w) / 2;
    if (box_x < 2) box_x = 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    mvwaddstr(box, 2, 4, "Keyboard Layout (X11): ");
    mvwaddstr(box, 4, 4, "Timezone (zoneinfo):   ");

    /* Render Keyboard Layout */
    if (focus == FLD_KB) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 2, 27, st->keyboard, 25);
    if (focus == FLD_KB) wattroff(box, COLOR_PAIR(CP_SELECTED));
    
    /* Render Timezone */
    if (focus == FLD_TZ) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 4, 27, st->timezone, 25);
    if (focus == FLD_TZ) wattroff(box, COLOR_PAIR(CP_SELECTED));

    wrefresh(box);
    delwin(box);

    row += box_h + 1;

    /* ── Error message ── */
    if (err_msg[0]) {
        ui_center(stdscr, row, err_msg, CP_DANGER, A_BOLD);
    }
    row += 2;
    
    /* Common hints */
    ui_center(stdscr, row++, "Examples for Keyboard: us, uk, fr, de, es, it", CP_DIM, 0);
    ui_center(stdscr, row++, "Examples for Timezone: UTC, America/New_York, Europe/London", CP_DIM, 0);

    /* ── Buttons ── */
    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

int screen_locale(installer_state_t *st)
{
    const char *footer[] = {
        "TAB/↑↓ Navigate",
        "ENTER Edit/Select",
        "← Back",
        "Q Quit",
    };
    
    int focus = FLD_KB;
    char err_msg[128] = {0};
    
    while (1) {
        ui_draw_header("Step 3 of 6  —  Locale & Timezone");
        ui_draw_footer(footer, 4);
        draw_locale_screen(st, focus, err_msg);

        int ch = getch();
        err_msg[0] = '\0'; /* Clear error on keypress */

        switch (ch) {
            case KEY_UP: case 'k':
                focus = (focus - 1 + NUM_FIELDS) % NUM_FIELDS;
                break;
            case KEY_DOWN: case 'j': case '\t':
                focus = (focus + 1) % NUM_FIELDS;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (focus == FLD_KB) {
                    int bw = ui_body_width();
                    int box_x = (bw - 60) / 2;
                    if (box_x < 2) box_x = 2;
                    WINDOW *box = newwin(8, 60, ui_body_top() + 4, box_x);
                    ui_readline(box, 2, 27, 25, st->keyboard, MAX_KB_LEN, false);
                    delwin(box);
                } else if (focus == FLD_TZ) {
                    int bw = ui_body_width();
                    int box_x = (bw - 60) / 2;
                    if (box_x < 2) box_x = 2;
                    WINDOW *box = newwin(8, 60, ui_body_top() + 4, box_x);
                    ui_readline(box, 4, 27, 25, st->timezone, MAX_TZ_LEN, false);
                    delwin(box);
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
                    /* Validate */
                    if (strlen(st->keyboard) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Keyboard layout cannot be empty.");
                        focus = FLD_KB;
                    } else if (strlen(st->timezone) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Timezone cannot be empty.");
                        focus = FLD_TZ;
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
