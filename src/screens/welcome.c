#include "welcome.h"
#include "../ui.h"
#include "../installer.h"
#include <ncurses.h>
#include <string.h>

/* ─── ASCII banner (7-row, fits in 70 cols) ──────────────────────────────── */
static const char *LOGO[] = {
    " ██████╗  █████╗ ██████╗ ██████╗  █████╗ ██████╗  ██████╗ ██╗   ██╗███████╗",
    " ██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔═══██╗██║   ██║██╔════╝",
    " ██████╔╝███████║██████╔╝██████╔╝███████║██████╔╝██║   ██║██║   ██║███████╗",
    " ██╔══██╗██╔══██║██╔══██╗██╔══██╗██╔══██║██╔══██╗██║   ██║██║   ██║╚════██║",
    " ██████╔╝██║  ██║██║  ██║██████╔╝██║  ██║██║  ██║╚██████╔╝╚██████╔╝███████║",
    " ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚══════╝",
    NULL
};

/* Smaller banner for narrow terminals (< 82 cols) */
static const char *LOGO_SM[] = {
    "██████╗  █████╗ ██████╗ ██████╗  █████╗ ██████╗  ██████╗ ██╗   ██╗███████╗",
    "██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔═══██╗██║   ██║██╔════╝",
    "██████╔╝███████║██████╔╝██████╔╝███████║██████╔╝██║   ██║██║   ██║███████╗",
    "██╔══██╗██╔══██║██╔══██╗██╔══██╗██╔══██║██╔══██╗██║   ██║██║   ██║╚════██║",
    "╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚══════╝",
    NULL
};

static const char *STEPS[] = {
    " Disk & partition selection",
    " User account & password",
    " Keyboard layout & timezone",
    " RPM package selection",
    " Binary & dotfile injection",
    " Ignition file generation",
};
#define NSTEPS ((int)(sizeof(STEPS)/sizeof(STEPS[0])))

/* ─── Draw ───────────────────────────────────────────────────────────────── */

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

    /* ── Logo ── */
    const char **logo = (boxw >= 80) ? LOGO : LOGO_SM;
    int logo_rows = 0;
    while (logo[logo_rows]) logo_rows++;
    int logo_w = (int)strlen(logo[0]);
    int logo_x = (boxw - logo_w) / 2;
    if (logo_x < 1) logo_x = 1;
    int logo_y = 2;

    for (int i = 0; i < logo_rows; i++) {
        wattron(box, COLOR_PAIR(CP_ACCENT) | A_BOLD);
        mvwaddstr(box, logo_y + i, logo_x, logo[i]);
        wattroff(box, COLOR_PAIR(CP_ACCENT) | A_BOLD);
    }

    /* ── Subtitle ── */
    int row = logo_y + logo_rows + 1;
    ui_center(box, row, "Fedora CoreOS  ◆  Ignition File Generator", CP_DIM, A_BOLD);
    row++;

    /* ── Divider ── */
    row++;
    wattron(box, COLOR_PAIR(CP_BORDER));
    mvwhline(box, row, 1, ACS_HLINE, boxw - 2);
    wattroff(box, COLOR_PAIR(CP_BORDER));
    row++;

    /* ── Step list ── */
    const char *hdr = "This installer will guide you through:";
    int hdr_x = (boxw - (int)strlen(hdr)) / 2;
    if (hdr_x < 2) hdr_x = 2;
    wattron(box, COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvwaddstr(box, row, hdr_x, hdr);
    wattroff(box, COLOR_PAIR(CP_NORMAL) | A_BOLD);
    row++;

    int list_x = (boxw / 2) - 18;
    if (list_x < 3) list_x = 3;

    for (int i = 0; i < NSTEPS; i++) {
        row++;
        wattron(box, COLOR_PAIR(CP_KEY) | A_BOLD);
        mvwaddch(box, row, list_x, ACS_DIAMOND);
        wattroff(box, COLOR_PAIR(CP_KEY) | A_BOLD);
        wattron(box, COLOR_PAIR(CP_NORMAL));
        mvwaddstr(box, row, list_x + 2, STEPS[i]);
        wattroff(box, COLOR_PAIR(CP_NORMAL));
    }

    /* ── Press-enter prompt ── */
    row += 2;
    wattron(box, COLOR_PAIR(CP_BORDER));
    mvwhline(box, row, 1, ACS_HLINE, boxw - 2);
    wattroff(box, COLOR_PAIR(CP_BORDER));
    row += 2;

    ui_button(box, row, (boxw - 20) / 2, "  ENTER  Begin  ", true);

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
