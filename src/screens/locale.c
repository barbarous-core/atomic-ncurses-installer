#include "locale.h"
#include "../ui.h"
#include "../installer.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

#define NUM_FIELDS 7
#define FLD_KB     0
#define FLD_TZ     1
#define FLD_USER   2
#define FLD_PASS   3
#define FLD_CONF   4
#define BTN_BACK   5
#define BTN_NEXT   6

static int get_system_list(const char *cmd, char ***list)
{
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    char line[128];
    int count = 0;
    int capacity = 128;
    *list = malloc(capacity * sizeof(char *));
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (count >= capacity) {
            capacity *= 2;
            *list = realloc(*list, capacity * sizeof(char *));
        }
        (*list)[count++] = strdup(line);
    }
    pclose(fp);
    return count;
}

static void free_system_list(char **list, int count)
{
    for (int i = 0; i < count; i++) free(list[i]);
    free(list);
}

static void draw_locale_screen(const installer_state_t *st, int focus, const char *err_msg)
{
    ui_clear_body();

    int bw  = ui_body_width();
    int top = ui_body_top();

    /* ── Instructions ── */
    int row = top + 2;
    ui_center(stdscr, row, "System Configuration: locale, timezone and user creation.", CP_NORMAL, A_BOLD);
    row += 2;

    /* ── Form box ── */
    int box_w = 60;
    int box_h = 14;
    int box_x = (bw - box_w) / 2;
    if (box_x < 2) box_x = 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    mvwaddstr(box, 2, 4, "Keyboard Layout : ");
    mvwaddstr(box, 4, 4, "Timezone (zoneinfo):   ");

    /* Render Keyboard Layout */
    if (focus == FLD_KB) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 2, 27, st->keyboard, 25);
    if (focus == FLD_KB) wattroff(box, COLOR_PAIR(CP_SELECTED));
    
    /* Render Timezone */
    if (focus == FLD_TZ) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 4, 27, st->timezone[0] ? st->timezone : "(select)", 25);
    if (focus == FLD_TZ) wattroff(box, COLOR_PAIR(CP_SELECTED));

    mvwaddstr(box, 6, 4, "Username: ");
    if (focus == FLD_USER) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 6, 27, st->username[0] ? st->username : "(enter)", 25);
    if (focus == FLD_USER) wattroff(box, COLOR_PAIR(CP_SELECTED));

    mvwaddstr(box, 8, 4, "Password: ");
    if (focus == FLD_PASS) wattron(box, COLOR_PAIR(CP_SELECTED));
    if (st->password_hash[0]) {
        mvwaddstr(box, 8, 27, "********");
    } else {
        mvwaddstr(box, 8, 27, "(set)");
    }
    if (focus == FLD_PASS) wattroff(box, COLOR_PAIR(CP_SELECTED));

    mvwaddstr(box, 10, 4, "Confirm:  ");
    if (focus == FLD_CONF) wattron(box, COLOR_PAIR(CP_SELECTED));
    if (st->password_hash[0]) {
        mvwaddstr(box, 10, 27, "********");
    } else {
        mvwaddstr(box, 10, 27, "(confirm)");
    }
    if (focus == FLD_CONF) wattroff(box, COLOR_PAIR(CP_SELECTED));

    wrefresh(box);
    delwin(box);

    row += box_h + 1;

    /* ── Error message ── */
    if (err_msg[0]) {
        ui_center(stdscr, row, err_msg, CP_DANGER, A_BOLD);
    }
    row += 2;
    
    /* Common hints */
    ui_center(stdscr, row++, "Examples for Keyboard: us, uk, fr, de, es, it", CP_DIM, 0);
    ui_center(stdscr, row++, "Examples for Timezone: UTC, America/New_York, Europe/London", CP_DIM, 0);

    /* ── Buttons ── */
    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

int screen_locale(installer_state_t *st)
{
    const char *footer[] = {
        "TAB/↑↓ Navigate",
        "ENTER Edit/Select",
        "← Back",
        "Q Quit",
    };
    
    int focus = FLD_KB;
    char err_msg[128] = {0};
    
    while (1) {
        ui_draw_header("Step 1 of 6  —  Locale & Timezone");
        ui_draw_footer(footer, 4);
        draw_locale_screen(st, focus, err_msg);

        int ch = getch();
        err_msg[0] = '\0'; /* Clear error on keypress */

        switch (ch) {
            case KEY_UP: case 'k':
                focus = (focus - 1 + NUM_FIELDS) % NUM_FIELDS;
                break;
            case KEY_DOWN: case 'j': case '\t':
                focus = (focus + 1) % NUM_FIELDS;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (focus == FLD_KB) {
                    char **layouts = NULL;
                    int count = get_system_list("localectl list-x11-keymap-layouts", &layouts);
                    if (count > 0) {
                        int initial = 0;
                        for (int i = 0; i < count; i++) {
                            if (strcmp(layouts[i], st->keyboard) == 0) {
                                initial = i;
                                break;
                            }
                        }
                        int sel = ui_menu("Select Keyboard Layout", (const char **)layouts, count, initial);
                        if (sel >= 0) {
                            strncpy(st->keyboard, layouts[sel], MAX_KB_LEN - 1);
                        }
                        free_system_list(layouts, count);
                    } else {
                        WINDOW *box = newwin(8, 60, ui_body_top() + 4, (ui_body_width() - 60) / 2);
                        ui_readline(box, 2, 27, 25, st->keyboard, MAX_KB_LEN, false);
                        delwin(box);
                    }
                } else if (focus == FLD_TZ) {
                    char **tzs = NULL;
                    int count = get_system_list("timedatectl list-timezones", &tzs);
                    if (count > 0) {
                        int initial = 0;
                        for (int i = 0; i < count; i++) {
                            if (strcmp(tzs[i], st->timezone) == 0) {
                                initial = i;
                                break;
                            }
                        }
                        int sel = ui_menu("Select Timezone", (const char **)tzs, count, initial);
                        if (sel >= 0) {
                            strncpy(st->timezone, tzs[sel], MAX_TZ_LEN - 1);
                        }
                        free_system_list(tzs, count);
                    } else {
                        WINDOW *box = newwin(12, 60, ui_body_top() + 4, (ui_body_width() - 60) / 2);
                        ui_readline(box, 4, 27, 25, st->timezone, MAX_TZ_LEN, false);
                        delwin(box);
                    }
                } else if (focus == FLD_USER) {
                    WINDOW *box = newwin(12, 60, ui_body_top() + 4, (ui_body_width() - 60) / 2);
                    ui_readline(box, 6, 27, 25, st->username, MAX_USER_LEN, false);
                    delwin(box);
                } else if (focus == FLD_PASS || focus == FLD_CONF) {
                    char pass1[MAX_USER_LEN] = {0};
                    char pass2[MAX_USER_LEN] = {0};
                    WINDOW *pbox = newwin(14, 60, ui_body_top() + 4, (ui_body_width() - 60) / 2);
                    
                    ui_readline(pbox, 8, 27, 25, pass1, MAX_USER_LEN, true);
                    ui_readline(pbox, 10, 27, 25, pass2, MAX_USER_LEN, true);
                    
                    if (strlen(pass1) < 4) {
                        snprintf(err_msg, sizeof(err_msg), "Password too short (min 4 chars).");
                        focus = FLD_PASS;
                    } else if (strcmp(pass1, pass2) != 0) {
                        snprintf(err_msg, sizeof(err_msg), "Passwords do not match!");
                        st->password_hash[0] = '\0';
                        focus = FLD_PASS;
                    } else {
                        /* Hash the password using openssl */
                        char cmd[512];
                        snprintf(cmd, sizeof(cmd), "openssl passwd -6 \"%s\"", pass1);
                        FILE *fp = popen(cmd, "r");
                        if (fp) {
                            if (fgets(st->password_hash, MAX_HASH_LEN, fp)) {
                                st->password_hash[strcspn(st->password_hash, "\n")] = '\0';
                            }
                            pclose(fp);
                        }
                        /* If correct, move to Next button */
                        if (st->password_hash[0]) focus = BTN_NEXT;
                        else focus = FLD_PASS;
                    }
                    delwin(pbox);
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
                    /* Validate */
                    if (strlen(st->keyboard) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Keyboard layout cannot be empty.");
                        focus = FLD_KB;
                    } else if (strlen(st->timezone) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Timezone cannot be empty.");
                        focus = FLD_TZ;
                    } else if (strlen(st->username) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Username cannot be empty.");
                        focus = FLD_USER;
                    } else if (strlen(st->password_hash) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Password is required.");
                        focus = FLD_PASS;
                    } else {
                        return NAV_NEXT;
                    }
                }
                break;
            case KEY_RESIZE:
                break;
            default:
                break;
        }
    }
}
