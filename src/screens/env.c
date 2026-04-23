#include "env.h"
#include "../ui.h"
#include "../installer.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[64];
    char size[16];
    char type[16];
    char model[64];
    bool is_partition;
} dev_entry_t;

#define MAX_DEVS 128

static int enumerate_devices(dev_entry_t *devs, int max)
{
    /* Use lsblk -l to get a flat list of all block devices and partitions */
    FILE *fp = popen("lsblk -n -l -o NAME,SIZE,TYPE,MODEL -e 7 2>/dev/null", "r");
    if (!fp) return 0;

    int count = 0;
    char line[512];
    while (count < max && fgets(line, sizeof(line), fp)) {
        dev_entry_t *d = &devs[count];
        memset(d, 0, sizeof(*d));
        
        char type[32] = {0};
        char model[256] = {0};
        
        /* Parse NAME, SIZE, TYPE, MODEL */
        if (sscanf(line, "%63s %15s %31s %255[^\n]", d->name, d->size, type, model) < 3)
            continue;
        
        strncpy(d->type, type, sizeof(d->type)-1);
        strncpy(d->model, model[0] ? model : "-", sizeof(d->model)-1);
        d->is_partition = (strcmp(type, "part") == 0);
        
        count++;
    }
    pclose(fp);
    return count;
}

static void draw_env_screen(dev_entry_t *devs, int count, int selected, int scroll)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    ui_center(stdscr, top + 2, "Environment Check & Storage Selection", CP_NORMAL, A_BOLD);
    ui_center(stdscr, top + 4, "Identify the target disk or partition for installation.", CP_DIM, A_NORMAL);

    int list_top = top + 7;
    int list_bot = LINES - FOOTER_H - 2;
    int list_h = list_bot - list_top;
    
    /* Column headers */
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    mvaddstr(list_top - 1, 4,  "NAME");
    mvaddstr(list_top - 1, 15, "SIZE");
    mvaddstr(list_top - 1, 25, "TYPE");
    mvaddstr(list_top - 1, 35, "MODEL");
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    
    if (count == 0) {
        ui_center(stdscr, list_top + 2, "No storage devices detected!", CP_DANGER, A_BOLD);
    }

    for (int i = 0; i < list_h && (i + scroll) < count; i++) {
        int idx = i + scroll;
        bool sel = (idx == selected);
        int row = list_top + i;
        
        if (sel) attron(COLOR_PAIR(CP_SELECTED));
        
        /* Clear row background */
        mvhline(row, 0, ' ', bw);
        
        /* Render device info */
        mvprintw(row, 2, "%s %-10s %-8s %-8s %s", 
                 sel ? "→" : " ",
                 devs[idx].name, 
                 devs[idx].size, 
                 devs[idx].type, 
                 devs[idx].model);
                 
        if (sel) attroff(COLOR_PAIR(CP_SELECTED));
    }
}

int screen_env(installer_state_t *st)
{
    dev_entry_t devs[MAX_DEVS];
    int count = enumerate_devices(devs, MAX_DEVS);
    int selected = 0;
    int scroll = 0;
    
    const char *footer[] = {
        "↑↓   Navigate",
        "ENTER Select",
        "←    Back",
        "Q    Quit",
    };

    while (1) {
        ui_draw_header("Step 3 of 6  —  Environment Check");
        ui_draw_footer(footer, 4);
        draw_env_screen(devs, count, selected, scroll);
        
        int ch = getch();
        int list_h = LINES - FOOTER_H - (ui_body_top() + 7);
        if (list_h < 1) list_h = 1;
        
        switch (ch) {
            case KEY_UP: case 'k':
                if (selected > 0) {
                    selected--;
                    if (selected < scroll) scroll = selected;
                }
                break;
            case KEY_DOWN: case 'j': case '\t':
                if (selected < count - 1) {
                    selected++;
                    if (selected >= scroll + list_h) scroll = selected - list_h + 1;
                }
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (count > 0) {
                    /* Ensure we don't overflow st->disk */
                    char full_path[128];
                    snprintf(full_path, sizeof(full_path), "/dev/%s", devs[selected].name);
                    strncpy(st->disk, full_path, MAX_DISK_LEN - 1);
                    st->disk[MAX_DISK_LEN - 1] = '\0';
                    return NAV_NEXT;
                }
                break;
            case KEY_RESIZE:
                break;
        }
    }
}
