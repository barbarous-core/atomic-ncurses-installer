#include "packages.h"
#include "../ui.h"
#include "../installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define MAX_DISCOVERED_RPMS 128

typedef struct {
    char filename[256];
    bool selected;
} rpm_entry_t;

static int discover_rpms(rpm_entry_t *rpms, int max_rpms)
{
    /* In a real environment, we'd search the ISO paths.
     * For now, check a local ./rpms folder, or predefined paths. */
    const char *search_paths[] = {
        "/run/media/liveuser/fedora-coreos/barbarous-assets/rpms",
        "/run/media/iso/barbarous-assets/rpms",
        "./rpms",
        NULL
    };

    int count = 0;
    for (int i = 0; search_paths[i] != NULL; i++) {
        DIR *d = opendir(search_paths[i]);
        if (d) {
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL && count < max_rpms) {
                if (dir->d_type == DT_REG) {
                    const char *ext = strrchr(dir->d_name, '.');
                    if (ext && strcmp(ext, ".rpm") == 0) {
                        strncpy(rpms[count].filename, dir->d_name, sizeof(rpms[count].filename) - 1);
                        rpms[count].selected = true; /* Default to selected */
                        count++;
                    }
                }
            }
            closedir(d);
            if (count > 0) break; /* Found RPMs in one path, stop searching */
        }
    }
    return count;
}

static void draw_packages_screen(rpm_entry_t *rpms, int count, int selected_idx, int scroll, bool focus_btn, bool focus_next)
{
    ui_clear_body();

    int bw  = ui_body_width();
    int top = ui_body_top();

    /* ── Instructions ── */
    int row = top + 1;
    ui_center(stdscr, row, "Select extra RPM packages to install on first boot.", CP_NORMAL, A_BOLD);
    row += 2;

    /* ── List Box ── */
    int box_w = bw - 4;
    int box_h = LINES - FOOTER_H - row - 3;
    if (box_h < 5) return;
    int box_x = 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    int list_rows = box_h - 2;

    if (count == 0) {
        ui_center(box, list_rows / 2 + 1, "No RPM packages found in assets.", CP_DIM, A_BOLD);
    } else {
        for (int i = 0; i < list_rows && (i + scroll) < count; i++) {
            int idx  = i + scroll;
            int r  = i + 1;
            bool sel = (idx == selected_idx) && !focus_btn;

            int pair = sel ? CP_SELECTED : CP_NORMAL;
            attr_t at = sel ? A_BOLD : 0;

            wattron(box, COLOR_PAIR(pair) | at);
            mvwhline(box, r, 1, ' ', box_w - 2);
            
            /* Checkbox */
            mvwaddstr(box, r, 2, rpms[idx].selected ? "[X]" : "[ ]");
            
            /* Filename */
            mvwaddnstr(box, r, 6, rpms[idx].filename, box_w - 8);
            
            wattroff(box, COLOR_PAIR(pair) | at);
        }

        /* Scroll indicators */
        if (scroll > 0) {
            wattron(box, COLOR_PAIR(CP_DIM) | A_BOLD);
            mvwaddstr(box, 0, box_w - 8, " (more)");
            wattroff(box, COLOR_PAIR(CP_DIM) | A_BOLD);
        }
        if (scroll + list_rows < count) {
            wattron(box, COLOR_PAIR(CP_DIM) | A_BOLD);
            mvwaddstr(box, box_h - 1, box_w - 8, " (more)");
            wattroff(box, COLOR_PAIR(CP_DIM) | A_BOLD);
        }
    }

    wrefresh(box);
    delwin(box);

    /* ── Buttons ── */
    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus_btn && !focus_next);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus_btn && focus_next);

    refresh();
}

int screen_packages(installer_state_t *st)
{
    const char *footer[] = {
        "↑↓ Navigate",
        "SPACE Toggle",
        "TAB Focus Buttons",
        "ENTER Confirm",
        "Q Quit",
    };
    
    rpm_entry_t rpms[MAX_DISCOVERED_RPMS];
    int count = discover_rpms(rpms, MAX_DISCOVERED_RPMS);
    
    /* Pre-fill with existing selections if returning */
    if (st->rpm_count > 0 && count > 0) {
        for (int i = 0; i < count; i++) {
            rpms[i].selected = false;
            for (int j = 0; j < st->rpm_count; j++) {
                if (strcmp(rpms[i].filename, st->rpms[j]) == 0) {
                    rpms[i].selected = true;
                    break;
                }
            }
        }
    }
    
    int selected_idx = 0;
    int scroll = 0;
    bool focus_btn = false;
    bool focus_next = true;
    
    while (1) {
        int list_rows = LINES - FOOTER_H - (ui_body_top() + 6);
        if (list_rows < 1) list_rows = 1;

        ui_draw_header("Step 4 of 6  —  RPM Packages");
        ui_draw_footer(footer, 5);
        draw_packages_screen(rpms, count, selected_idx, scroll, focus_btn, focus_next);

        int ch = getch();

        switch (ch) {
            case KEY_UP: case 'k':
                if (focus_btn) {
                    focus_btn = false;
                } else if (selected_idx > 0) {
                    selected_idx--;
                    if (selected_idx < scroll) scroll = selected_idx;
                }
                break;
            case KEY_DOWN: case 'j':
                if (!focus_btn && count > 0) {
                    if (selected_idx < count - 1) {
                        selected_idx++;
                        if (selected_idx >= scroll + list_rows)
                            scroll = selected_idx - list_rows + 1;
                    } else {
                        focus_btn = true;
                    }
                }
                break;
            case '\t':
                focus_btn = !focus_btn;
                break;
            case KEY_LEFT: case 'h':
                if (focus_btn) focus_next = false;
                break;
            case KEY_RIGHT: case 'l':
                if (focus_btn) focus_next = true;
                break;
            case ' ':
                if (!focus_btn && count > 0) {
                    rpms[selected_idx].selected = !rpms[selected_idx].selected;
                } else if (focus_btn) {
                    if (focus_next) goto next;
                    else return NAV_PREV;
                }
                break;
            case '\n': case KEY_ENTER:
                if (focus_btn) {
                    if (focus_next) goto next;
                    else return NAV_PREV;
                } else {
                    /* If ENTER is pressed while on the list, act as Next */
                    goto next;
                }
                break;
            case 'q': case 'Q':
                return NAV_QUIT;
            case KEY_RESIZE:
                break;
            default:
                break;
        }
        continue;
        
next:
        /* Save selections */
        st->rpm_count = 0;
        for (int i = 0; i < count; i++) {
            if (rpms[i].selected && st->rpm_count < MAX_RPMS) {
                strncpy(st->rpms[st->rpm_count], rpms[i].filename, MAX_RPM_LEN - 1);
                st->rpm_count++;
            }
        }
        return NAV_NEXT;
    }
}
