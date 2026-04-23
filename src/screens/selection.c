#include "selection.h"
#include "../ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CATS 20

static int get_categories(char cats[MAX_CATS][64]) {
    FILE *f = fopen("assets/matrix.csv", "r");
    if (!f) return 0;

    char line[1024];
    int count = 0;
    /* skip header */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        char cat[64] = {0};
        if (*p == '"') {
            p++;
            char *end = strchr(p, '"');
            if (end) {
                int len = end - p;
                if (len >= 64) len = 63;
                strncpy(cat, p, len);
                cat[len] = '\0';
            }
        } else {
            char *comma = strchr(p, ',');
            if (comma) {
                int len = comma - p;
                if (len >= 64) len = 63;
                strncpy(cat, p, len);
                cat[len] = '\0';
            }
        }
        
        if (cat[0] == '\0') continue;

        bool found = false;
        for (int i=0; i<count; i++) {
            if (strcmp(cats[i], cat) == 0) {
                found = true;
                break;
            }
        }
        if (!found && count < MAX_CATS) {
            strncpy(cats[count], cat, 63);
            count++;
        }
    }
    fclose(f);
    return count;
}

#define FOCUS_LIST 0
#define BTN_BACK   1
#define BTN_NEXT   2
#define NFOCUS     3

static void draw_selection_screen(char cats[MAX_CATS][64], int count, int selected, bool checked[MAX_CATS], int focus)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    int row = top + 2;
    ui_center(stdscr, row, "RPM and Bin Selection", CP_NORMAL, A_BOLD);
    row += 2;
    ui_center(stdscr, row, "Select package categories to include in your installation.", CP_DIM, A_NORMAL);
    row += 3;

    int box_w = 60;
    int box_h = count + 4;
    int box_x = (bw - box_w) / 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    for (int i = 0; i < count; i++) {
        bool is_sel = (i == selected && focus == FOCUS_LIST);
        if (is_sel) wattron(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        
        mvwprintw(box, 2 + i, 4, "%s [%s] %-40s", 
                 is_sel ? "→" : " ", 
                 checked[i] ? "X" : " ",
                 cats[i]);
        
        if (is_sel) wattroff(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }

    wrefresh(box);
    delwin(box);

    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

int screen_selection(installer_state_t *st)
{
    (void)st;
    char cats[MAX_CATS][64];
    int count = get_categories(cats);
    bool checked[MAX_CATS] = {0};
    /* By default check all? Or none? Let's check all for now */
    for(int i=0; i<count; i++) checked[i] = true;

    int selected = 0;
    int focus = FOCUS_LIST;

    const char *footer[] = {
        "↑↓   Navigate",
        "SPACE Toggle",
        "ENTER Next",
        "←    Back",
        "Q    Quit",
    };

    while (1) {
        ui_draw_header("Step 4 of 6  —  RPM and Bin Selection");
        ui_draw_footer(footer, 5);
        draw_selection_screen(cats, count, selected, checked, focus);

        int ch = getch();
        switch (ch) {
            case KEY_UP: case 'k':
                if (focus == FOCUS_LIST) {
                    if (selected > 0) selected--;
                } else {
                    focus = FOCUS_LIST;
                }
                break;
            case KEY_DOWN: case 'j':
                if (focus == FOCUS_LIST) {
                    if (selected < count - 1) selected++;
                    else focus = BTN_BACK;
                } else {
                    focus = FOCUS_LIST;
                }
                break;
            case '\t':
                focus = (focus + 1) % NFOCUS;
                break;
            case ' ':
                if (focus == FOCUS_LIST) {
                    checked[selected] = !checked[selected];
                }
                break;
            case KEY_LEFT:
                if (focus == BTN_NEXT) focus = BTN_BACK;
                else return NAV_PREV;
                break;
            case KEY_RIGHT:
                if (focus == BTN_BACK) focus = BTN_NEXT;
                break;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER:
                if (focus == FOCUS_LIST) {
                    /* Just toggle or move to buttons? Usually ENTER on list moves to buttons or next screen */
                    focus = BTN_NEXT;
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
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
