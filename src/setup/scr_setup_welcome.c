#include "scr_setup_welcome.h"
#include "setup.h"
#include "../common/ui.h"
#include <ncurses.h>

static const char *STEPS[] = {
    " ISO Insertion & Asset Path Configuration",
    " Matrix Configuration Loading (CSV/URL)",
    " RPM Layering & System Update",
    " Binary Deployment to System Paths",
    " Dotfiles Synchronization & Installation",
};
#define NSTEPS ((int)(sizeof(STEPS) / sizeof(STEPS[0])))

static void draw_welcome(void) {
  ui_clear_body();

  int bh = ui_body_height();
  int bw = ui_body_width();
  int top = ui_body_top();

  int boxw = bw - 4;
  int boxh = bh - 2;
  int boxy = top + 1;
  int boxx = 2;
  if (boxw < 20 || boxh < 8)
    return;

  WINDOW *box = newwin(boxh, boxw, boxy, boxx);
  wbkgd(box, COLOR_PAIR(CP_NORMAL));
  ui_box(box, CP_BORDER);

  int start_row = (boxh - NSTEPS) / 2 - 2;
  const char *hdr = "Barbarous OS Post-Install Setup";
  ui_center(box, start_row, hdr, CP_ACCENT, A_BOLD);

  for (int i = 0; i < NSTEPS; i++) {
    int r = start_row + 3 + i;
    int list_x = (boxw / 2) - 22;
    if (list_x < 3)
      list_x = 3;

    wattron(box, COLOR_PAIR(CP_KEY) | A_BOLD);
    mvwaddch(box, r, list_x, ACS_DIAMOND);
    wattroff(box, COLOR_PAIR(CP_KEY) | A_BOLD);
    wattron(box, COLOR_PAIR(CP_NORMAL));
    mvwaddstr(box, r, list_x + 2, STEPS[i]);
    wattroff(box, COLOR_PAIR(CP_NORMAL));
  }

  ui_button(box, boxh - 2, (boxw - 20) / 2, "  ENTER  Start Setup  ", true);

  wrefresh(box);
  delwin(box);
}

int screen_setup_welcome(setup_state_t *st) {
  (void)st;
  const char *footer[] = {
      "ENTER Start",
      "Q     Quit",
  };

  ui_draw_header("Welcome to Setup");
  ui_draw_footer(footer, 2);
  draw_welcome();

  while (1) {
    int ch = getch();
    switch (ch) {
    case '\n':
    case KEY_ENTER:
    case ' ':
      return NAV_NEXT;
    case 'q':
    case 'Q':
    case 27:
      return NAV_QUIT;
    case KEY_RESIZE:
      ui_draw_header("Welcome to Setup");
      ui_draw_footer(footer, 2);
      draw_welcome();
      break;
    default:
      break;
    }
  }
}
