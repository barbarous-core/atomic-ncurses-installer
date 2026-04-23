#include "disk.h"
#include "../ui.h"
#include "../installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ─── Disk entry ─────────────────────────────────────────────────────────── */
#define MAX_DISKS 32

typedef struct {
    char name[32];    /* sda, nvme0n1              */
    char path[64];    /* /dev/sda                  */
    char size[16];    /* 256.0G                    */
    char model[64];   /* Samsung SSD 860 EVO       */
    char tran[16];    /* sata, nvme, usb           */
} disk_entry_t;

/* ─── Enumerate block devices via lsblk ─────────────────────────────────── */
/*
 * lsblk -d -e 7 -n -o NAME,SIZE,TRAN,MODEL
 *   -d   disks only (no partitions)
 *   -e 7 exclude loop devices (major 7)
 *   -n   no header
 */
static int enumerate_disks(disk_entry_t *disks, int max)
{
    FILE *fp = popen("lsblk -d -e 7 -n -o NAME,SIZE,TRAN,MODEL 2>/dev/null", "r");
    if (!fp) return 0;

    int count = 0;
    char line[256];

    while (count < max && fgets(line, sizeof(line), fp)) {
        /* Strip trailing newline */
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) < 2) continue;

        disk_entry_t *d = &disks[count];
        memset(d, 0, sizeof(*d));

        /* Parse: NAME(col 0-13) SIZE(col 14-21) TRAN(col 22-29) MODEL(rest) */
        /* Use sscanf — lsblk output is whitespace-separated                  */
        char rest[200] = {0};
        sscanf(line, "%31s %15s %15s %199[^\n]",
               d->name, d->size, d->tran, rest);

        /* Model may be empty; tran might actually be the model if no tran col*/
        /* Copy model from rest, trimming leading spaces */
        char *m = rest;
        while (*m == ' ') m++;
        strncpy(d->model, m[0] ? m : "—", sizeof(d->model) - 1);

        /* Build /dev/NAME path */
        snprintf(d->path, sizeof(d->path), "/dev/%s", d->name);

        count++;
    }
    pclose(fp);
    return count;
}

/* ─── Layout constants ───────────────────────────────────────────────────── */
#define COL_NAME   2
#define COL_SIZE   20
#define COL_TRAN   30
#define COL_MODEL  40

/* ─── Draw the disk list ─────────────────────────────────────────────────── */
static void draw_disk_screen(disk_entry_t *disks, int count,
                              int selected, int scroll,
                              const installer_state_t *st)
{
    ui_clear_body();

    int bw  = ui_body_width();
    int top = ui_body_top();

    /* ── Warning banner ── */
    int brow = top + 1;
    attron(COLOR_PAIR(CP_DANGER) | A_BOLD);
    mvhline(brow, 0, ' ', bw);
    const char *warn = "  ⚠  WARNING: All data on the selected disk will be permanently erased!  ⚠";
    int wx = (bw - (int)strlen(warn)) / 2;
    if (wx < 0) wx = 0;
    mvaddstr(brow, wx, warn);
    attroff(COLOR_PAIR(CP_DANGER) | A_BOLD);

    /* ── Prompt ── */
    brow += 2;
    const char *prompt = "Select the disk where Fedora CoreOS will be installed:";
    attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvaddstr(brow, (bw - (int)strlen(prompt)) / 2, prompt);
    attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);

    /* ── Column headers ── */
    brow += 2;
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    mvaddstr(brow, COL_NAME,  "DEVICE");
    mvaddstr(brow, COL_SIZE,  "SIZE");
    mvaddstr(brow, COL_TRAN,  "TRANSPORT");
    mvaddstr(brow, COL_MODEL, "MODEL");
    brow++;
    attron(COLOR_PAIR(CP_BORDER));
    mvhline(brow, 1, ACS_HLINE, bw - 2);
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);

    /* ── Disk rows ── */
    int list_top  = brow + 1;
    int list_bot  = LINES - FOOTER_H - 4;   /* leave room for buttons */
    int list_rows = list_bot - list_top;

    if (count == 0) {
        attron(COLOR_PAIR(CP_DANGER) | A_BOLD);
        mvaddstr(list_top + 1, COL_NAME, "No block devices found.");
        attroff(COLOR_PAIR(CP_DANGER) | A_BOLD);
    }

    for (int i = 0; i < list_rows && (i + scroll) < count; i++) {
        int idx  = i + scroll;
        int row  = list_top + i;
        bool sel = (idx == selected);

        /* Arrow indicator */
        if (sel) {
            attron(COLOR_PAIR(CP_KEY) | A_BOLD);
            mvaddch(row, COL_NAME - 2, ACS_RARROW);
            attroff(COLOR_PAIR(CP_KEY) | A_BOLD);
        } else {
            mvaddch(row, COL_NAME - 2, ' ');
        }

        int pair = sel ? CP_SELECTED : CP_NORMAL;
        attr_t  at = sel ? A_BOLD : 0;

        attron(COLOR_PAIR(pair) | at);
        /* Clear full row first */
        mvhline(row, COL_NAME - 1, ' ', bw - COL_NAME + 1);

        mvaddstr(row, COL_NAME,  disks[idx].path);
        mvaddstr(row, COL_SIZE,  disks[idx].size);
        mvaddstr(row, COL_TRAN,  disks[idx].tran);

        /* Clip model to available width */
        int model_w = bw - COL_MODEL - 2;
        if (model_w > 0) {
            char clip[128];
            strncpy(clip, disks[idx].model, sizeof(clip) - 1);
            clip[sizeof(clip) - 1] = '\0';
            if ((int)strlen(clip) > model_w) clip[model_w] = '\0';
            mvaddstr(row, COL_MODEL, clip);
        }
        attroff(COLOR_PAIR(pair) | at);
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
        snprintf(info, sizeof(info), "Selected: %s  (%s)  %s",
                 disks[selected].path,
                 disks[selected].size,
                 disks[selected].model);
        attron(COLOR_PAIR(CP_ACCENT));
        int ix = (bw - (int)strlen(info)) / 2;
        if (ix < 2) ix = 2;
        mvaddstr(btn_row - 1 + 0, ix, info); /* overlap divider with info */
        attroff(COLOR_PAIR(CP_ACCENT));
    }

    /* ── Buttons ── */
    /* Show previously chosen disk if any */
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

    disk_entry_t disks[MAX_DISKS];
    int count = enumerate_disks(disks, MAX_DISKS);

    /* Pre-select previously chosen disk if any */
    int selected = 0;
    if (st->disk[0]) {
        for (int i = 0; i < count; i++) {
            if (strcmp(disks[i].path, st->disk) == 0) {
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
                             disks[selected].path, disks[selected].size);
                    if (ui_confirm("⚠  Confirm Disk Destruction", msg)) {
                        strncpy(st->disk, disks[selected].path,
                                MAX_DISK_LEN - 1);
                        return NAV_NEXT;
                    }
                    /* Redraw after dialog */
                    ui_draw_header("Step 1 of 6  —  Select Installation Disk");
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
                ui_draw_header("Step 1 of 6  —  Select Installation Disk");
                ui_draw_footer(footer, 4);
                break;

            default:
                break;
        }

        draw_disk_screen(disks, count, selected, scroll, st);
    }
}
