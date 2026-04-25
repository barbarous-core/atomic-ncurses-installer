#include "scr_edition.h"
#include "ui.h"
#include "installer.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_EDITIONS 10

static const char *EDITIONS[] = {
    "Core",
    "Station",
    "Studio",
    "Edge",
    "Lab",
    "Touch"
};
#define NEDITIONS (int)(sizeof(EDITIONS)/sizeof(EDITIONS[0]))

#define FOCUS_LIST 0
#define BTN_BACK   1
#define BTN_NEXT   2
#define NFOCUS     3

static void draw_edition_screen(int selected, int focus)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    int row = top + 2;
    ui_center(stdscr, row, "Select Barbarous OS Edition", CP_NORMAL, A_BOLD);
    row += 2;
    ui_center(stdscr, row, "Each edition comes with a curated set of tools and configs.", CP_DIM, A_NORMAL);
    row += 3;

    int box_w = 40;
    int box_h = NEDITIONS + 4;
    int box_x = (bw - box_w) / 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    for (int i = 0; i < NEDITIONS; i++) {
        bool is_sel = (i == selected);
        if (is_sel) wattron(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        
        mvwprintw(box, 2 + i, 4, "%s %-20s", is_sel ? "→" : " ", EDITIONS[i]);
        
        if (is_sel) wattroff(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }

    wrefresh(box);
    delwin(box);

    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

int screen_edition(installer_state_t *st)
{
    int selected = 0;
    int focus = FOCUS_LIST;
    
    /* Pre-select current edition if it matches */
    for (int i = 0; i < NEDITIONS; i++) {
        if (strcmp(st->edition, EDITIONS[i]) == 0) {
            selected = i;
            break;
        }
    }

    const char *footer[] = {
        "↑↓   Navigate",
        "ENTER Select",
        "←    Back",
        "Q    Quit",
    };

    while (1) {
        ui_draw_header("Step 3 of 6  —  Edition Selection");
        ui_draw_footer(footer, 4);
        draw_edition_screen(selected, focus);

        int ch = getch();
        switch (ch) {
            case KEY_UP: case 'k':
                if (focus == FOCUS_LIST) {
                    selected = (selected - 1 + NEDITIONS) % NEDITIONS;
                } else {
                    focus = FOCUS_LIST;
                }
                break;
            case KEY_DOWN: case 'j':
                if (focus == FOCUS_LIST) {
                    if (selected == NEDITIONS - 1) {
                        focus = BTN_BACK;
                    } else {
                        selected++;
                    }
                } else {
                    focus = FOCUS_LIST;
                }
                break;
            case '\t':
                focus = (focus + 1) % NFOCUS;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case KEY_RIGHT:
                if (focus == BTN_BACK) focus = BTN_NEXT;
                else if (focus == BTN_NEXT) focus = BTN_BACK;
                break;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (focus == FOCUS_LIST) {
                    strncpy(st->edition, EDITIONS[selected], MAX_EDITION_LEN - 1);
                    focus = BTN_NEXT;
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
                    strncpy(st->edition, EDITIONS[selected], MAX_EDITION_LEN - 1);
                    return NAV_NEXT;
                }
                break;
            case KEY_RESIZE:
                break;
            default:
                break;
        }
    }
}
