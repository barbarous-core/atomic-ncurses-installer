#include "edition.h"
#include "../ui.h"
#include "../installer.h"
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

static void draw_edition_screen(int selected)
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
    refresh();
}

int screen_edition(installer_state_t *st)
{
    int selected = 0;
    
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
        ui_draw_header("Step 4 of 6  —  Edition Selection");
        ui_draw_footer(footer, 4);
        draw_edition_screen(selected);

        int ch = getch();
        switch (ch) {
            case KEY_UP: case 'k':
                selected = (selected - 1 + NEDITIONS) % NEDITIONS;
                break;
            case KEY_DOWN: case 'j': case '\t':
                selected = (selected + 1) % NEDITIONS;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                strncpy(st->edition, EDITIONS[selected], MAX_EDITION_LEN - 1);
                return NAV_NEXT;
            case KEY_RESIZE:
                break;
            default:
                break;
        }
    }
}
