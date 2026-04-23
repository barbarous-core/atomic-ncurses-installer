#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include "installer.h"
#include "ui.h"
#include "screens/welcome.h"
#include "screens/locale.h"
#include "screens/env.h"
#include "screens/edition.h"
#include "screens/config.h"
#include "screens/disk.h"
#include "screens/generate.h"

int main(void)
{
    setlocale(LC_ALL, "");
    /* ── ncurses init ── */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(50);   /* fast ESC detection */

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Error: your terminal does not support colors.\n");
        return 1;
    }
    start_color();
    ui_init_colors();

    /* Black background for the whole screen */
    bkgd(COLOR_PAIR(CP_NORMAL));
    clear();
    refresh();

    /* ── Installer state ── */
    installer_state_t state;
    installer_init(&state);

    /* ── Screen router ──
     * Each screen returns NAV_NEXT, NAV_PREV, or NAV_QUIT.
     * As we add screens they slot in here.
     */
    int screen = 0;
    int nav    = NAV_NEXT;

    while (1) {
        switch (screen) {
            case 0:
                nav = screen_welcome(&state);
                break;
            case 1:
                nav = screen_locale(&state);
                break;
            case 2:
                nav = screen_env(&state);
                break;
            case 3:
                nav = screen_edition(&state);
                break;
            case 4:
                nav = screen_config(&state);
                break;
            case 5:
                nav = screen_disk(&state);
                break;
            case 6:
                nav = screen_generate(&state);
                break;

            default:
                nav = NAV_QUIT;
                break;
        }

        if (nav == NAV_QUIT) break;
        if (nav == NAV_NEXT) screen++;
        if (nav == NAV_PREV && screen > 0) screen--;
    }

    /* ── Cleanup ── */
    endwin();

    if (nav == NAV_QUIT && screen > 0) {
        /* User finished all screens – IGN generation goes here later */
        printf("\n[barbarous-install] IGN file written to: %s\n\n",
               state.output_path);
    } else {
        printf("\n[barbarous-install] Installation cancelled.\n\n");
    }

    return 0;
}
