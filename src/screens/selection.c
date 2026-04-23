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

typedef struct {
    char name[64];
    char type[16];
    char category[64];
    bool selected;
} tool_db_entry_t;

static int load_all_tools(tool_db_entry_t *tools, int max)
{
    FILE *f = fopen("assets/matrix.csv", "r");
    if (!f) return 0;

    char line[1024];
    int count = 0;
    if (!fgets(line, sizeof(line), f)) { fclose(f); return 0; }

    while (fgets(line, sizeof(line), f) && count < max) {
        char *p = line;
        char cat[64] = {0};
        
        /* 1. Category */
        if (*p == '"') {
            p++;
            char *end = strchr(p, '"');
            if (end) {
                int len = end - p;
                if (len >= 64) len = 63;
                strncpy(cat, p, len);
                cat[len] = '\0';
                p = end + 2; 
            }
        } else {
            char *comma = strchr(p, ',');
            if (comma) {
                int len = comma - p;
                if (len >= 64) len = 63;
                strncpy(cat, p, len);
                cat[len] = '\0';
                p = comma + 1;
            }
        }
        strncpy(tools[count].category, cat, 63);

        /* 2. Tool Name */
        char *comma = strchr(p, ',');
        if (!comma) continue;
        int nlen = comma - p;
        if (nlen >= 64) nlen = 63;
        strncpy(tools[count].name, p, nlen);
        tools[count].name[nlen] = '\0';
        p = comma + 1;

        /* 3. Type */
        comma = strchr(p, ',');
        if (!comma) continue;
        int tlen = comma - p;
        if (tlen >= 16) tlen = 15;
        strncpy(tools[count].type, p, tlen);
        tools[count].type[tlen] = '\0';
        
        tools[count].selected = true; // Default All
        count++;
    }
    fclose(f);
    return count;
}

static void draw_selection_screen(char cats[MAX_CATS][64], int cat_count, 
                                  tool_db_entry_t *all_tools, int tool_count,
                                  int selected_cat, int focus)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    ui_center(stdscr, top + 2, "RPM and Bin Selection", CP_NORMAL, A_BOLD);
    ui_center(stdscr, top + 4, "Select package categories to include in your installation.", CP_DIM, A_NORMAL);

    int box_w = bw - 10;
    int box_h = cat_count + 4;
    int box_x = (bw - box_w) / 2;
    int box_y = top + 6;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    for (int i = 0; i < cat_count; i++) {
        int sel_in_cat = 0;
        int tot_in_cat = 0;
        for (int j = 0; j < tool_count; j++) {
            if (strcmp(all_tools[j].category, cats[i]) == 0) {
                tot_in_cat++;
                if (all_tools[j].selected) sel_in_cat++;
            }
        }

        char state_icon = ' ';
        if (sel_in_cat == tot_in_cat) state_icon = 'A';
        else if (sel_in_cat == 0) state_icon = ' ';
        else state_icon = 'P';

        bool is_sel = (i == selected_cat && focus == FOCUS_LIST);
        if (is_sel) wattron(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        
        mvwprintw(box, 2 + i, 4, "[%c] %-40s (%d/%d)", 
                 state_icon, cats[i], sel_in_cat, tot_in_cat);
        
        if (is_sel) wattroff(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }

    wrefresh(box);
    delwin(box);

    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);
    refresh();
}

static void screen_category_detail(const char *category, tool_db_entry_t *all_tools, int tool_count)
{
    /* We work on the pointers directly in all_tools */
    int indices[128];
    int count = 0;
    for (int i = 0; i < tool_count && count < 128; i++) {
        if (strcmp(all_tools[i].category, category) == 0) {
            indices[count++] = i;
        }
    }
    if (count == 0) return;

    int selected = 0;
    while (1) {
        /* Map tool_row_t layout for drawing but use real data */
        int sw = ui_body_width();
        int sh = ui_body_height();
        int bw = sw - 10;
        int bh = count + 6;
        if (bh > sh - 4) bh = sh - 4;
        int bx = (sw - bw) / 2;
        int by = (sh - bh) / 2 + ui_body_top();

        WINDOW *win = newwin(bh, bw, by, bx);
        wbkgd(win, COLOR_PAIR(CP_NORMAL));
        ui_box(win, CP_ACCENT);
        mvwprintw(win, 0, 2, " [ %s ] ", category);
        
        int row = 2;
        wattron(win, A_BOLD | COLOR_PAIR(CP_ACCENT));
        mvwprintw(win, row++, 4, "%-4s %-30s %-10s", "SEL", "NAME", "TYPE");
        mvwhline(win, row++, 4, ACS_HLINE, bw - 8);
        wattroff(win, A_BOLD | COLOR_PAIR(CP_ACCENT));

        for (int i = 0; i < count; i++) {
            bool is_sel = (i == selected);
            if (is_sel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            int idx = indices[i];
            mvwprintw(win, row + i, 4, "[%s]  %-30s %-10s", 
                     all_tools[idx].selected ? "X" : " ",
                     all_tools[idx].name,
                     all_tools[idx].type);
            if (is_sel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        }
        mvwprintw(win, bh - 2, (bw - 30) / 2, "SPACE: Toggle | ENTER: Back");
        wrefresh(win);
        delwin(win);

        int ch = getch();
        if (ch == '\n' || ch == KEY_ENTER || ch == 27) break;
        if (ch == ' ') all_tools[indices[selected]].selected = !all_tools[indices[selected]].selected;
        if (ch == KEY_UP || ch == 'k') { if (selected > 0) selected--; }
        if (ch == KEY_DOWN || ch == 'j') { if (selected < count - 1) selected++; }
    }
}

int screen_selection(installer_state_t *st)
{
    (void)st;
    tool_db_entry_t all_tools[256];
    int tool_count = load_all_tools(all_tools, 256);
    
    char cats[MAX_CATS][64];
    int cat_count = get_categories(cats);

    int selected_cat = 0;
    int focus = FOCUS_LIST;

    const char *footer[] = {
        "↑↓ Navigate",
        "SPACE All/None",
        "ENTER Detail",
        "← Back",
        "Q Quit",
    };

    while (1) {
        ui_draw_header("Step 4 of 6  —  RPM and Bin Selection");
        ui_draw_footer(footer, 5);
        draw_selection_screen(cats, cat_count, all_tools, tool_count, selected_cat, focus);

        int ch = getch();
        switch (ch) {
            case KEY_UP: case 'k':
                if (focus == FOCUS_LIST) {
                    if (selected_cat > 0) selected_cat--;
                } else focus = FOCUS_LIST;
                break;
            case KEY_DOWN: case 'j':
                if (focus == FOCUS_LIST) {
                    if (selected_cat < cat_count - 1) selected_cat++;
                    else focus = BTN_BACK;
                } else focus = FOCUS_LIST;
                break;
            case '\t': focus = (focus + 1) % NFOCUS; break;
            case ' ':
                if (focus == FOCUS_LIST) {
                    /* Toggle All/None for the category */
                    bool some_sel = false;
                    for(int i=0; i<tool_count; i++) {
                        if(strcmp(all_tools[i].category, cats[selected_cat]) == 0 && all_tools[i].selected) {
                            some_sel = true; break;
                        }
                    }
                    for(int i=0; i<tool_count; i++) {
                        if(strcmp(all_tools[i].category, cats[selected_cat]) == 0) {
                            all_tools[i].selected = !some_sel;
                        }
                    }
                }
                break;
            case KEY_LEFT:
                if (focus == BTN_NEXT) focus = BTN_BACK;
                else return NAV_PREV;
                break;
            case KEY_RIGHT:
                if (focus == BTN_BACK) focus = BTN_NEXT;
                break;
            case 'q': case 'Q': return NAV_QUIT;
            case '\n': case KEY_ENTER:
                if (focus == FOCUS_LIST) {
                    screen_category_detail(cats[selected_cat], all_tools, tool_count);
                } else if (focus == BTN_BACK) return NAV_PREV;
                else if (focus == BTN_NEXT) return NAV_NEXT;
                break;
            case KEY_RESIZE: break;
            default: break;
        }
    }
}
