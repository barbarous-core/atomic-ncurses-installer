#include "scr_setup_edition.h"
#include "../common/ui.h"
#include <stdio.h>
#include <string.h>

#define MAX_EDITIONS 20
#define MAX_TOOLS 1024

typedef struct {
    char name[64];
    int col_idx;
} edition_t;

typedef struct {
    char name[64];
    char type[16];
    bool belongs_to[MAX_EDITIONS];
    bool selected[MAX_EDITIONS];
} tool_entry_t;

static int parse_matrix(const char *matrix_path, edition_t *editions, int *num_editions, tool_entry_t *tools, int *num_tools)
{
    FILE *f = fopen(matrix_path, "r");
    if (!f) return -1;

    char line[2048];
    *num_editions = 0;
    *num_tools = 0;

    /* Parse header */
    if (fgets(line, sizeof(line), f)) {
        char *p = line;
        int col = 0;
        while (*p) {
            char *start = p;
            char *end = strchr(p, ',');
            if (!end) end = p + strlen(p);
            
            int len = end - start;
            while (len > 0 && (start[len-1] == '\n' || start[len-1] == '\r')) len--;
            
            char val[64];
            int cp_len = len < 64 ? len : 63;
            strncpy(val, start, cp_len);
            val[cp_len] = '\0';
            
            if (col == 0 && strcasecmp(val, "Category") != 0) { fclose(f); return -2; }
            if (col == 1 && strcasecmp(val, "File") != 0 && strcasecmp(val, "Name") != 0) { fclose(f); return -2; }
            if (col == 2 && strcasecmp(val, "Type") != 0) { fclose(f); return -2; }
            
            if (col >= 3 && *num_editions < MAX_EDITIONS && len > 0) {
                strncpy(editions[*num_editions].name, val, sizeof(editions[*num_editions].name) - 1);
                editions[*num_editions].name[sizeof(editions[*num_editions].name) - 1] = '\0';
                editions[*num_editions].col_idx = col;
                (*num_editions)++;
            }
            if (*end == '\0') break;
            p = end + 1;
            col++;
        }
        
        if (col < 3) { fclose(f); return -2; }
    }

    /* Parse rows */
    while (fgets(line, sizeof(line), f) && *num_tools < MAX_TOOLS) {
        char *p = line;
        char name[64] = {0};
        char type[16] = {0};
        
        char *col_starts[MAX_EDITIONS + 3];
        int num_cols = 0;
        while (*p && num_cols < MAX_EDITIONS + 3) {
            col_starts[num_cols++] = p;
            char *end = strchr(p, ',');
            if (!end) break;
            *end = '\0';
            p = end + 1;
        }

        if (num_cols > 1) {
            strncpy(name, col_starts[1], sizeof(name) - 1);
        }
        if (num_cols > 2) {
            strncpy(type, col_starts[2], sizeof(type) - 1);
        }

        if (name[0]) {
            tool_entry_t *t = &tools[*num_tools];
            strncpy(t->name, name, sizeof(t->name) - 1);
            strncpy(t->type, type, sizeof(t->type) - 1);
            for (int i = 0; i < MAX_EDITIONS; i++) {
                t->belongs_to[i] = false;
                t->selected[i] = false;
            }

            for (int i = 0; i < *num_editions; i++) {
                int c_idx = editions[i].col_idx;
                if (c_idx < num_cols) {
                    char *val = col_starts[c_idx];
                    if (val[0] == 'x' || val[0] == 'X') {
                        t->belongs_to[i] = true;
                    }
                }
            }
            (*num_tools)++;
        }
    }

    fclose(f);
    return *num_tools;
}

static void screen_edition_detail(edition_t *edition, int ed_idx, tool_entry_t *all_tools, int tool_count)
{
    int indices[MAX_TOOLS];
    int count = 0;
    for (int i = 0; i < tool_count; i++) {
        if (all_tools[i].belongs_to[ed_idx]) {
            indices[count++] = i;
        }
    }
    if (count == 0) return;

    int selected = 0;
    int scroll = 0;

    while (1) {
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
        mvwprintw(win, 0, 2, " [ %s Packages ] ", edition->name);
        
        int row = 2;
        wattron(win, A_BOLD | COLOR_PAIR(CP_ACCENT));
        mvwprintw(win, row++, 4, "%-4s %-30s %-10s", "SEL", "NAME", "TYPE");
        mvwhline(win, row++, 4, ACS_HLINE, bw - 8);
        wattroff(win, A_BOLD | COLOR_PAIR(CP_ACCENT));

        int visible = bh - 6;
        for (int i = 0; i < visible && (i + scroll) < count; i++) {
            int idx = i + scroll;
            bool is_sel = (idx == selected);
            if (is_sel) wattron(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            int tool_idx = indices[idx];
            mvwprintw(win, row + i, 4, "[%s]  %-30s %-10s", 
                     all_tools[tool_idx].selected[ed_idx] ? "X" : " ",
                     all_tools[tool_idx].name,
                     all_tools[tool_idx].type);
            if (is_sel) wattroff(win, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        }

        mvwprintw(win, bh - 2, (bw - 30) / 2, "SPACE: Toggle | ENTER: Back");
        wrefresh(win);
        delwin(win);

        int ch = getch();
        if (ch == '\n' || ch == KEY_ENTER || ch == 27) break;
        if (ch == ' ') all_tools[indices[selected]].selected[ed_idx] = !all_tools[indices[selected]].selected[ed_idx];
        if (ch == KEY_UP || ch == 'k') { 
            if (selected > 0) {
                selected--;
                if (selected < scroll) scroll = selected;
            }
        }
        if (ch == KEY_DOWN || ch == 'j') { 
            if (selected < count - 1) {
                selected++;
                if (selected >= scroll + visible) scroll = selected - visible + 1;
            }
        }
    }
}

static void draw_edition_screen(edition_t *editions, int num_editions, tool_entry_t *tools, int num_tools, int selected)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    int row = top + 2;
    ui_center(stdscr, row, "Select Barbarous Setup Edition", CP_NORMAL, A_BOLD);
    row += 2;
    ui_center(stdscr, row, "Select or toggle editions to customize your installation.", CP_DIM, A_NORMAL);
    row += 2;

    int box_w = 60;
    int box_h = num_editions + 7;
    int box_x = (bw - box_w) / 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    /* Headers */
    wattron(box, COLOR_PAIR(CP_ACCENT) | A_BOLD);
    mvwprintw(box, 1, 4, "%-15s %-10s %-10s %-10s", "EDITION", "RPMs", "BINs", "DOTs");
    mvwhline(box, 2, 2, ACS_HLINE, box_w - 4);
    wattroff(box, COLOR_PAIR(CP_ACCENT) | A_BOLD);

    for (int i = 0; i < num_editions; i++) {
        int sel_in_ed = 0;
        int tot_in_ed = 0;
        int rpms = 0, bins = 0, dots = 0;
        
        for (int j = 0; j < num_tools; j++) {
            if (tools[j].belongs_to[i]) {
                tot_in_ed++;
                if (tools[j].selected[i]) {
                    sel_in_ed++;
                    if (strcmp(tools[j].type, "rpm") == 0) rpms++;
                    else if (strcmp(tools[j].type, "bin") == 0) bins++;
                    else if (strcmp(tools[j].type, "dotfile") == 0) dots++;
                }
            }
        }

        char state_icon = ' ';
        if (tot_in_ed > 0 && sel_in_ed == tot_in_ed) state_icon = 'A';
        else if (sel_in_ed == 0) state_icon = ' ';
        else state_icon = 'P';

        bool is_sel = (i == selected);
        if (is_sel) wattron(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        
        mvwprintw(box, 3 + i, 2, "%s [%c] %-10s %-10d %-10d %-10d", 
                  is_sel ? "→" : " ",
                  state_icon,
                  editions[i].name, 
                  rpms, 
                  bins, 
                  dots);
        
        if (is_sel) wattroff(box, COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }

    /* Totals Footer */
    mvwhline(box, box_h - 4, 1, ACS_HLINE, box_w - 2);
    
    int total_rpms = 0;
    int total_bins = 0;
    int total_dots = 0;
    for (int j = 0; j < num_tools; j++) {
        bool sel = false;
        for (int i = 0; i < num_editions; i++) {
            if (tools[j].belongs_to[i] && tools[j].selected[i]) { sel = true; break; }
        }
        if (sel) {
            if (strcmp(tools[j].type, "rpm") == 0) total_rpms++;
            else if (strcmp(tools[j].type, "bin") == 0) total_bins++;
            else if (strcmp(tools[j].type, "dotfile") == 0) total_dots++;
        }
    }
    
    wattron(box, A_BOLD | COLOR_PAIR(CP_ACCENT));
    mvwprintw(box, box_h - 2, 4, "Total Unique Selected:  %d RPMs,  %d BINs", total_rpms, total_bins);
    wattroff(box, A_BOLD | COLOR_PAIR(CP_ACCENT));

    wrefresh(box);
    delwin(box);

    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", selected == num_editions);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", selected == num_editions + 1);

    refresh();
}

int screen_setup_edition(setup_state_t *st)
{
    edition_t editions[MAX_EDITIONS];
    int num_editions = 0;
    
    tool_entry_t tools[MAX_TOOLS];
    int num_tools = 0;
    
    int res = parse_matrix(st->matrix_path, editions, &num_editions, tools, &num_tools);
    
    if (res == -2) {
        char err[512];
        snprintf(err, sizeof(err), "Invalid matrix format. Expected header:\nCategory,File,Type,Edition1...\nFound in: %s", st->matrix_path);
        ui_msgbox("Error", err, CP_DANGER);
        return NAV_PREV;
    } else if (res < 0) {
        char err[512];
        snprintf(err, sizeof(err), "Could not open matrix CSV file:\n%s", st->matrix_path);
        ui_msgbox("Error", err, CP_DANGER);
        return NAV_PREV;
    } else if (res == 0 || num_editions == 0) {
        char err[512];
        snprintf(err, sizeof(err), "Could not parse editions from matrix CSV:\n%s", st->matrix_path);
        ui_msgbox("Error", err, CP_DANGER);
        return NAV_PREV;
    }

    /* Set default selections based on st->edition if set, else none */
    if (st->edition[0]) {
        int target_ed = -1;
        for (int i = 0; i < num_editions; i++) {
            if (strcmp(st->edition, editions[i].name) == 0) {
                target_ed = i;
                break;
            }
        }
        if (target_ed >= 0) {
            for (int j = 0; j < num_tools; j++) {
                if (tools[j].belongs_to[target_ed]) tools[j].selected[target_ed] = true;
            }
        }
    }

    int selected = 0;

    const char *footer[] = {
        "↑↓    Navigate",
        "SPACE All/None",
        "ENTER Detail/Next",
        "←     Back",
        "Q     Quit",
    };

    while (1) {
        ui_draw_header("Step 3 of 4  —  Edition Selection");
        ui_draw_footer(footer, 5);
        draw_edition_screen(editions, num_editions, tools, num_tools, selected);

        int ch = getch();
        switch (ch) {
            case KEY_UP: case 'k':
                selected = (selected - 1 + num_editions + 2) % (num_editions + 2);
                break;
            case KEY_DOWN: case 'j': case '\t':
                selected = (selected + 1) % (num_editions + 2);
                break;
            case KEY_LEFT: case 27:
                if (selected == num_editions + 1) selected = num_editions;
                else return NAV_PREV;
                break;
            case KEY_RIGHT:
                if (selected == num_editions) selected = num_editions + 1;
                break;
            case ' ':
                if (selected < num_editions) {
                    bool some_sel = false;
                    for (int j = 0; j < num_tools; j++) {
                        if (tools[j].belongs_to[selected] && tools[j].selected[selected]) {
                            some_sel = true; break;
                        }
                    }
                    for (int j = 0; j < num_tools; j++) {
                        if (tools[j].belongs_to[selected]) {
                            tools[j].selected[selected] = !some_sel;
                        }
                    }
                }
                break;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER:
                if (selected < num_editions) {
                    screen_edition_detail(&editions[selected], selected, tools, num_tools);
                } else if (selected == num_editions) {
                    return NAV_PREV;
                } else if (selected == num_editions + 1) {
                    /* Save to state */
                    st->rpm_count = 0;
                    st->bin_count = 0;
                    st->dotfile_count = 0;
                    
                    /* Generate composite edition name for UI */
                    st->edition[0] = '\0';
                    int ed_count = 0;
                    for (int i = 0; i < num_editions; i++) {
                        bool any_sel = false;
                        for (int j = 0; j < num_tools; j++) {
                            if (tools[j].belongs_to[i] && tools[j].selected[i]) {
                                any_sel = true; break;
                            }
                        }
                        if (any_sel) {
                            if (ed_count > 0) strncat(st->edition, "+", sizeof(st->edition) - strlen(st->edition) - 1);
                            strncat(st->edition, editions[i].name, sizeof(st->edition) - strlen(st->edition) - 1);
                            ed_count++;
                        }
                    }
                    if (ed_count == 0) strncpy(st->edition, "Custom", sizeof(st->edition) - 1);

                    for (int j = 0; j < num_tools; j++) {
                        bool sel = false;
                        for (int i = 0; i < num_editions; i++) {
                            if (tools[j].belongs_to[i] && tools[j].selected[i]) { sel = true; break; }
                        }
                        if (sel) {
                            if (strcmp(tools[j].type, "rpm") == 0 && st->rpm_count < MAX_RPMS) {
                                strncpy(st->rpms[st->rpm_count++], tools[j].name, MAX_RPM_LEN - 1);
                            } else if (strcmp(tools[j].type, "bin") == 0 && st->bin_count < MAX_BINS) {
                                strncpy(st->bins[st->bin_count++], tools[j].name, MAX_BIN_LEN - 1);
                            } else if (strcmp(tools[j].type, "dotfile") == 0 && st->dotfile_count < MAX_DOTFILES) {
                                strncpy(st->dotfiles[st->dotfile_count++], tools[j].name, MAX_DOTFILE_LEN - 1);
                            }
                        }
                    }
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
