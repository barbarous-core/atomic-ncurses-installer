#include "scr_dotfiles.h"
#include "ui.h"
#include "installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define MAX_DISCOVERED_DOTFILES 128

typedef struct {
    char filename[256];
    bool selected;
} dotfile_entry_t;

static int discover_dotfiles(dotfile_entry_t *dots, int max_dots)
{
    /* Search the ISO paths for dotfiles directory. */
    const char *search_paths[] = {
        "/run/media/liveuser/fedora-coreos/barbarous-assets/dotfiles",
        "/run/media/iso/barbarous-assets/dotfiles",
        "./dotfiles",
        NULL
    };

    int count = 0;
    for (int i = 0; search_paths[i] != NULL; i++) {
        DIR *d = opendir(search_paths[i]);
        if (d) {
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL && count < max_dots) {
                if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                    strncpy(dots[count].filename, dir->d_name, sizeof(dots[count].filename) - 1);
                    dots[count].selected = true; /* Default to selected */
                    count++;
                }
            }
            closedir(d);
            if (count > 0) break; /* Found dotfiles in one path, stop searching */
        }
    }
    return count;
}

static void draw_dotfiles_screen(dotfile_entry_t *dots, int count, int selected_idx, int scroll, bool focus_btn, bool focus_next)
{
    ui_clear_body();

    int bw  = ui_body_width();
    int top = ui_body_top();

    /* ── Instructions ── */
    int row = top + 1;
    ui_center(stdscr, row, "Select dotfiles to deploy to the user's home directory.", CP_NORMAL, A_BOLD);
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
        ui_center(box, list_rows / 2 + 1, "No dotfiles found in assets.", CP_DIM, A_BOLD);
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
            mvwaddstr(box, r, 2, dots[idx].selected ? "[X]" : "[ ]");
            
            /* Filename */
            mvwaddnstr(box, r, 6, dots[idx].filename, box_w - 8);
            
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

int screen_dotfiles(installer_state_t *st)
{
    const char *footer[] = {
        "↑↓ Navigate",
        "SPACE Toggle",
        "TAB Focus Buttons",
        "ENTER Confirm",
        "Q Quit",
    };
    
    dotfile_entry_t dots[MAX_DISCOVERED_DOTFILES];
    int count = discover_dotfiles(dots, MAX_DISCOVERED_DOTFILES);
    
    /* Pre-fill with existing selections if returning */
    if (st->dotfile_count > 0 && count > 0) {
        for (int i = 0; i < count; i++) {
            dots[i].selected = false;
            for (int j = 0; j < st->dotfile_count; j++) {
                if (strcmp(dots[i].filename, st->dotfiles[j]) == 0) {
                    dots[i].selected = true;
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

        ui_draw_header("Step 6 of 6  —  Dotfile Injection");
        ui_draw_footer(footer, 5);
        draw_dotfiles_screen(dots, count, selected_idx, scroll, focus_btn, focus_next);

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
                    dots[selected_idx].selected = !dots[selected_idx].selected;
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
        st->dotfile_count = 0;
        for (int i = 0; i < count; i++) {
            if (dots[i].selected && st->dotfile_count < MAX_DOTFILES) {
                strncpy(st->dotfiles[st->dotfile_count], dots[i].filename, MAX_DOTFILE_LEN - 1);
                st->dotfile_count++;
            }
        }
        return NAV_NEXT;
    }
}
