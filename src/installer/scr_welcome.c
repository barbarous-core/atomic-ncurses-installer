#include "scr_welcome.h"
#include "ui.h"
#include "installer.h"
#include <ncurses.h>
#include <string.h>

static const char *STEPS[] = {
    " Locale, Timezone & User Configuration",
    " Hostname & SSH Key Setup",
    " Barbarous OS Edition Selection",
    " Package & Binary Customization",
    " Target Disk Selection & Verification",
    " Final Summary & System Installation",
};
#define NSTEPS ((int)(sizeof(STEPS)/sizeof(STEPS[0])))


static void draw_welcome(void)
{
    ui_clear_body();

    int bh   = ui_body_height();
    int bw   = ui_body_width();
    int top  = ui_body_top();

    /* ── Box ── */
    int boxw = bw  - 4;
    int boxh = bh  - 2;
    int boxy = top + 1;
    int boxx = 2;
    if (boxw < 20 || boxh < 8) return;          /* terminal too small */

    WINDOW *box = newwin(boxh, boxw, boxy, boxx);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    /* ── Step list at the bottom ── */
    int start_row = boxh - NSTEPS - 6;
    const char *hdr = "This installer will guide you through:";
    int hdr_x = (boxw - (int)strlen(hdr)) / 2;
    if (hdr_x < 2) hdr_x = 2;
    
    wattron(box, COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvwaddstr(box, start_row, hdr_x, hdr);
    wattroff(box, COLOR_PAIR(CP_NORMAL) | A_BOLD);

    int list_x = (boxw / 2) - 18;
    if (list_x < 3) list_x = 3;

    for (int i = 0; i < NSTEPS; i++) {
        int r = start_row + 2 + i;
        wattron(box, COLOR_PAIR(CP_KEY) | A_BOLD);
        mvwaddch(box, r, list_x, ACS_DIAMOND);
        wattroff(box, COLOR_PAIR(CP_KEY) | A_BOLD);
        wattron(box, COLOR_PAIR(CP_NORMAL));
        mvwaddstr(box, r, list_x + 2, STEPS[i]);
        wattroff(box, COLOR_PAIR(CP_NORMAL));
    }


    ui_button(box, boxh - 2, (boxw - 20) / 2, "  ENTER  Begin  ", true);

    wrefresh(box);
    delwin(box);
}

/* ─── Public entry point ─────────────────────────────────────────────────── */

int screen_welcome(installer_state_t *st)
{
    (void)st;   /* nothing to read/write on welcome screen */

    const char *footer[] = {
        "ENTER Begin",
        "Q     Quit",
    };

    ui_draw_header("Welcome");
    ui_draw_footer(footer, 2);
    draw_welcome();

    while (1) {
        int ch = getch();
        switch (ch) {
            case '\n': case KEY_ENTER: case ' ':
                return NAV_NEXT;
            case 'q': case 'Q': case 27:   /* ESC */
                return NAV_QUIT;
            case KEY_RESIZE:
                ui_draw_header("Welcome");
                ui_draw_footer(footer, 2);
                draw_welcome();
                break;
            default:
                break;
        }
    }
}
