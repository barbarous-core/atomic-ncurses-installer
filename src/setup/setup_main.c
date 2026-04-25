#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>

#include "setup.h"
#include "../common/ui.h"
#include "scr_setup_welcome.h"
#include "scr_setup_paths.h"
#include "scr_setup_matrix.h"
#include "scr_setup_apply.h"

int main(void)
{
    setlocale(LC_ALL, "");
    srand(time(NULL));

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(50);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Error: your terminal does not support colors.\n");
        return 1;
    }
    start_color();
    ui_init_colors();

    bkgd(COLOR_PAIR(CP_NORMAL));
    clear();
    refresh();

    setup_state_t state;
    setup_init(&state);

    int screen = 0;
    int nav    = NAV_NEXT;

    while (1) {
        clear();
        refresh();
        switch (screen) {
            case 0:
                nav = screen_setup_welcome(&state);
                break;
            case 1:
                nav = screen_setup_paths(&state);
                break;
            case 2:
                nav = screen_setup_matrix(&state);
                break;
            case 3:
                nav = screen_setup_apply(&state);
                break;

            default:
                nav = NAV_QUIT;
                break;
        }

        if (nav == NAV_QUIT) break;
        if (nav == NAV_NEXT) screen++;
        if (nav == NAV_PREV && screen > 0) screen--;
    }

    endwin();

    if (nav == NAV_QUIT && screen > 0) {
        printf("\n[barbarous-setup] Setup completed successfully.\n\n");
    } else {
        printf("\n[barbarous-setup] Setup cancelled.\n\n");
    }

    return 0;
}
