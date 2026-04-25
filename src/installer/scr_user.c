#include "scr_user.h"
#include "ui.h"
#include "installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_FIELDS 3
#define FLD_USER   0
#define FLD_PASS   1
#define FLD_CONF   2
#define BTN_BACK   3
#define BTN_NEXT   4

static void draw_user_screen(const installer_state_t *st, int focus, const char *err_msg, const char *pass_buf, const char *conf_buf)
{
    ui_clear_body();

    int bw  = ui_body_width();
    int top = ui_body_top();

    /* ── Instructions ── */
    int row = top + 2;
    ui_center(stdscr, row, "Create the primary user account for this system.", CP_NORMAL, A_BOLD);
    row += 2;

    /* ── Form box ── */
    int box_w = 50;
    int box_h = 10;
    int box_x = (bw - box_w) / 2;
    if (box_x < 2) box_x = 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    mvwaddstr(box, 2, 4, "Username: ");
    mvwaddstr(box, 4, 4, "Password: ");
    mvwaddstr(box, 6, 4, "Confirm:  ");

    /* Render Username */
    if (focus == FLD_USER) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 2, 14, st->username, 30);
    if (focus == FLD_USER) wattroff(box, COLOR_PAIR(CP_SELECTED));
    
    /* Render Password (masked) */
    char mask_pass[32] = {0};
    for (size_t i = 0; i < strlen(pass_buf) && i < 31; i++) mask_pass[i] = '*';
    if (focus == FLD_PASS) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 4, 14, mask_pass, 30);
    if (focus == FLD_PASS) wattroff(box, COLOR_PAIR(CP_SELECTED));

    /* Render Confirm (masked) */
    char mask_conf[32] = {0};
    for (size_t i = 0; i < strlen(conf_buf) && i < 31; i++) mask_conf[i] = '*';
    if (focus == FLD_CONF) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 6, 14, mask_conf, 30);
    if (focus == FLD_CONF) wattroff(box, COLOR_PAIR(CP_SELECTED));

    wrefresh(box);
    delwin(box);

    row += box_h + 1;

    /* ── Error message ── */
    if (err_msg[0]) {
        ui_center(stdscr, row, err_msg, CP_DANGER, A_BOLD);
    }
    row += 2;

    /* ── Buttons ── */
    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

/* ─── Hash generation ─── */
static bool generate_hash(const char *password, char *hash_out, int max_len)
{
    /* Create a secure temporary file */
    char tmptpl[] = "/tmp/barbarous_pw_XXXXXX";
    int fd = mkstemp(tmptpl);
    if (fd < 0) return false;
    
    write(fd, password, strlen(password));
    close(fd);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "openssl passwd -6 -stdin < %s 2>/dev/null", tmptpl);
    FILE *fp = popen(cmd, "r");
    bool ok = false;
    if (fp) {
        if (fgets(hash_out, max_len, fp) != NULL) {
            hash_out[strcspn(hash_out, "\n")] = '\0';
            ok = true;
        }
        pclose(fp);
    }
    
    /* Clean up securely */
    remove(tmptpl);
    return ok;
}

int screen_user(installer_state_t *st)
{
    const char *footer[] = {
        "TAB/↑↓ Navigate",
        "ENTER Edit/Select",
        "← Back",
        "Q Quit",
    };
    
    int focus = FLD_USER;
    char pass_buf[128] = {0};
    char conf_buf[128] = {0};
    char err_msg[128] = {0};
    
    while (1) {
        ui_draw_header("Step 2 of 6  —  User Account");
        ui_draw_footer(footer, 4);
        draw_user_screen(st, focus, err_msg, pass_buf, conf_buf);

        int ch = getch();
        err_msg[0] = '\0'; /* Clear error on keypress */

        switch (ch) {
            case KEY_UP: case 'k':
                focus = (focus - 1 + 5) % 5;
                break;
            case KEY_DOWN: case 'j': case '\t':
                focus = (focus + 1) % 5;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (focus == FLD_USER) {
                    /* Open input prompt for username */
                    int bw = ui_body_width();
                    int box_x = (bw - 50) / 2;
                    if (box_x < 2) box_x = 2;
                    WINDOW *box = newwin(10, 50, ui_body_top() + 4, box_x);
                    ui_readline(box, 2, 14, 30, st->username, MAX_USER_LEN, false);
                    delwin(box);
                } else if (focus == FLD_PASS) {
                    int bw = ui_body_width();
                    int box_x = (bw - 50) / 2;
                    if (box_x < 2) box_x = 2;
                    WINDOW *box = newwin(10, 50, ui_body_top() + 4, box_x);
                    ui_readline(box, 4, 14, 30, pass_buf, sizeof(pass_buf), true);
                    delwin(box);
                } else if (focus == FLD_CONF) {
                    int bw = ui_body_width();
                    int box_x = (bw - 50) / 2;
                    if (box_x < 2) box_x = 2;
                    WINDOW *box = newwin(10, 50, ui_body_top() + 4, box_x);
                    ui_readline(box, 6, 14, 30, conf_buf, sizeof(conf_buf), true);
                    delwin(box);
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
                    /* Validate */
                    if (strlen(st->username) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Username cannot be empty.");
                        focus = FLD_USER;
                    } else if (strlen(pass_buf) == 0) {
                        snprintf(err_msg, sizeof(err_msg), "Password cannot be empty.");
                        focus = FLD_PASS;
                    } else if (strcmp(pass_buf, conf_buf) != 0) {
                        snprintf(err_msg, sizeof(err_msg), "Passwords do not match!");
                        focus = FLD_CONF;
                    } else {
                        /* Generate hash */
                        ui_msgbox("Please Wait", "Generating password hash...", CP_ACCENT);
                        if (generate_hash(pass_buf, st->password_hash, MAX_HASH_LEN)) {
                            return NAV_NEXT;
                        } else {
                            snprintf(err_msg, sizeof(err_msg), "Failed to generate password hash.");
                        }
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
