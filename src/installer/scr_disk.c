#include "scr_disk.h"
#include "ui.h"
#include "installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ─── Disk entry ─────────────────────────────────────────────────────────── */
#define MAX_DISKS 32

typedef struct {
    char name_tree[64]; /* Tree string like "└─sda1" */
    char kname[32];     /* Kernel name like "sda1" */
    char label[32];
    char size[16];
    char type[16];
    char model[64];
} disk_tree_entry_t;

/* ─── Enumerate block devices via lsblk ─────────────────────────────────── */
/*
 * lsblk -d -e 7 -n -o NAME,SIZE,TRAN,MODEL
 *   -d   disks only (no partitions)
 *   -e 7 exclude loop devices (major 7)
 *   -n   no header
 */
static int get_pair_val(const char *line, const char *key, char *out, int max)
{
    char search[64];
    snprintf(search, sizeof(search), "%s=\"", key);
    char *p = strstr(line, search);
    if (!p) return 0;
    p += strlen(search);
    char *end = strchr(p, '\"');
    if (!end) return 0;
    int len = end - p;
    if (len >= max) len = max - 1;
    strncpy(out, p, len);
    out[len] = '\0';
    return 1;
}

static int enumerate_disks(disk_tree_entry_t *disks, int max)
{
    FILE *fp = popen("lsblk -n -P -o KNAME,LABEL,SIZE,TYPE,MODEL -e 7 2>/dev/null", "r");
    if (!fp) return 0;

    int count = 0;
    char line[512];
    while (count < max && fgets(line, sizeof(line), fp)) {
        disk_tree_entry_t *d = &disks[count];
        memset(d, 0, sizeof(*d));
        get_pair_val(line, "KNAME", d->kname, sizeof(d->kname));
        get_pair_val(line, "LABEL", d->label, sizeof(d->label));
        get_pair_val(line, "SIZE",  d->size,  sizeof(d->size));
        get_pair_val(line, "TYPE",  d->type,  sizeof(d->type));
        get_pair_val(line, "MODEL", d->model, sizeof(d->model));
        count++;
    }
    pclose(fp);

    fp = popen("lsblk -n -o NAME -e 7 2>/dev/null", "r");
    if (fp) {
        int i = 0;
        while (i < count && fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = '\0';
            strncpy(disks[i].name_tree, line, 63);
            i++;
        }
        pclose(fp);
    }
    return count;
}

/* ─── Layout constants ───────────────────────────────────────────────────── */
#define COL_NAME   4
#define COL_LABEL  25
#define COL_SIZE   45
#define COL_MODEL  58

/* ─── Draw the disk list ─────────────────────────────────────────────────── */
static void draw_disk_screen(disk_tree_entry_t *disks, int count,
                              int selected, int scroll,
                              const installer_state_t *st)
{
    ui_clear_body();
    int bw  = ui_body_width();
    int top = ui_body_top();
    (void)st;

    /* ── Warning banner ── */
    int brow = top + 1;
    attron(COLOR_PAIR(CP_DANGER) | A_BOLD);
    mvhline(brow, 0, ' ', bw);
    const char *warn = "  ⚠  WARNING: All data on the selected disk will be permanently erased!  ⚠";
    mvaddstr(brow, (bw - (int)strlen(warn)) / 2, warn);
    attroff(COLOR_PAIR(CP_DANGER) | A_BOLD);

    /* ── Prompt ── */
    brow += 2;
    ui_center(stdscr, brow, "Select the disk where Fedora CoreOS will be installed:", CP_NORMAL, A_BOLD);

    /* ── Column headers ── */
    brow += 2;
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    mvaddstr(brow, COL_NAME,  "NAME");
    mvaddstr(brow, COL_LABEL, "LABEL");
    mvaddstr(brow, COL_SIZE,  "SIZE");
    mvaddstr(brow, COL_MODEL, "MODEL");
    brow++;
    attron(COLOR_PAIR(CP_BORDER));
    mvhline(brow, 1, ACS_HLINE, bw - 2);
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);

    /* ── Disk rows ── */
    int list_top  = brow + 1;
    int list_bot  = LINES - FOOTER_H - 4;
    int list_rows = list_bot - list_top;

    for (int i = 0; i < list_rows && (i + scroll) < count; i++) {
        int idx  = i + scroll;
        int row  = list_top + i;
        bool sel = (idx == selected);

        if (sel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        mvhline(row, 0, ' ', bw);
        mvprintw(row, 2, "%s", sel ? "→" : " ");
        mvaddstr(row, COL_NAME,  disks[idx].name_tree);
        mvaddstr(row, COL_LABEL, disks[idx].label[0] ? disks[idx].label : "-");
        mvaddstr(row, COL_SIZE,  disks[idx].size);
        
        int model_w = bw - COL_MODEL - 2;
        if (model_w > 0) {
            char clip[128];
            strncpy(clip, disks[idx].model, sizeof(clip) - 1);
            clip[model_w] = '\0';
            mvaddstr(row, COL_MODEL, clip);
        }
        if (sel) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }

    /* Scroll indicator */
    if (scroll > 0) {
        attron(COLOR_PAIR(CP_DIM) | A_BOLD);
        mvaddstr(list_top - 1, bw - 8, " (more)");
        attroff(COLOR_PAIR(CP_DIM) | A_BOLD);
    }
    if (scroll + list_rows < count) {
        attron(COLOR_PAIR(CP_DIM) | A_BOLD);
        mvaddstr(list_bot, bw - 8, " (more)");
        attroff(COLOR_PAIR(CP_DIM) | A_BOLD);
    }

    /* ── Bottom divider ── */
    int btn_row = LINES - FOOTER_H - 2;
    attron(COLOR_PAIR(CP_BORDER));
    mvhline(btn_row - 1, 1, ACS_HLINE, bw - 2);
    attroff(COLOR_PAIR(CP_BORDER));

    /* ── Currently selected disk info ── */
    if (count > 0) {
        char info[256];
        snprintf(info, sizeof(info), "Selected: /dev/%s  (%s)  %s",
                 disks[selected].kname,
                 disks[selected].size,
                 disks[selected].model);
        attron(COLOR_PAIR(CP_ACCENT));
        int ix = (bw - (int)strlen(info)) / 2;
        if (ix < 2) ix = 2;
        mvaddstr(btn_row - 1, ix, info); 
        attroff(COLOR_PAIR(CP_ACCENT));
    }

    /* ── Buttons ── */
    (void)st;
    ui_button(stdscr, btn_row, 3,          "  ← Back  ", false);
    ui_button(stdscr, btn_row, bw - 16,    " Select → ", count > 0);

    refresh();
}

/* ─── Public entry point ─────────────────────────────────────────────────── */
int screen_disk(installer_state_t *st)
{
    const char *footer[] = {
        "↑↓ Navigate",
        "ENTER Select",
        "← Back",
        "Q Quit",
    };
    ui_draw_header("Step 5 of 6  —  Select Installation Disk");
    ui_draw_footer(footer, 4);

    disk_tree_entry_t disks[MAX_DISKS];
    int count = enumerate_disks(disks, MAX_DISKS);

    /* Pre-select previously chosen disk if any */
    int selected = 0;
    if (st->disk[0]) {
        for (int i = 0; i < count; i++) {
            char full_path[128];
            snprintf(full_path, sizeof(full_path), "/dev/%s", disks[i].kname);
            if (strcmp(full_path, st->disk) == 0) {
                selected = i;
                break;
            }
        }
    }
    int scroll = 0;

    /* How many rows visible */
    int list_rows = LINES - FOOTER_H - (ui_body_top() + 8);
    if (list_rows < 1) list_rows = 1;

    draw_disk_screen(disks, count, selected, scroll, st);

    while (1) {
        int ch = getch();

        switch (ch) {
            /* ── Navigation ── */
            case KEY_UP: case 'k':
                if (selected > 0) {
                    selected--;
                    if (selected < scroll) scroll = selected;
                }
                break;

            case KEY_DOWN: case 'j':
                if (selected < count - 1) {
                    selected++;
                    if (selected >= scroll + list_rows)
                        scroll = selected - list_rows + 1;
                }
                break;

            /* ── Select ── */
            case '\n': case KEY_ENTER: case ' ':
                if (count == 0) break;
                {
                    /* Confirmation dialog */
                    char msg[128];
                    snprintf(msg, sizeof(msg),
                             "Erase ALL data on %s (%s)?",
                             disks[selected].kname, disks[selected].size);
                    if (ui_confirm("⚠  Confirm Disk Destruction", msg)) {
                        snprintf(st->disk, MAX_DISK_LEN, "/dev/%s", disks[selected].kname);
                        return NAV_NEXT;
                    }
                    /* Redraw after dialog */
                    ui_draw_header("Step 5 of 6  —  Select Installation Disk");
                    ui_draw_footer(footer, 4);
                }
                break;

            /* ── Back ── */
            case KEY_LEFT: case KEY_BACKSPACE: case 27:
                return NAV_PREV;

            /* ── Quit ── */
            case 'q': case 'Q':
                return NAV_QUIT;

            /* ── Resize ── */
            case KEY_RESIZE:
                list_rows = LINES - FOOTER_H - (ui_body_top() + 8);
                if (list_rows < 1) list_rows = 1;
                ui_draw_header("Step 5 of 6  —  Select Installation Disk");
                ui_draw_footer(footer, 4);
                break;

            default:
                break;
        }

        draw_disk_screen(disks, count, selected, scroll, st);
    }
}
