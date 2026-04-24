#include "config.h"
#include "../ui.h"
#include "../installer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>


#define FLD_HOSTNAME 0
#define BTN_GEN_KEY  1
#define FLD_SSH_KEY  2
#define BTN_BACK     3
#define BTN_NEXT     4
#define NUM_FOCUS    5


static void draw_config_screen(const installer_state_t *st, int focus, const char *err_msg)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    int row = top + 2;
    ui_center(stdscr, row, "Hostname and SSH key", CP_NORMAL, A_BOLD);
    row += 2;
    ui_center(stdscr, row, "Set your hostname and provide an SSH public key for access.", CP_DIM, A_NORMAL);
    row += 1;

    int box_w = 74;
    int box_h = st->ssh_key[0] ? 29 : 10;
    int box_x = (bw - box_w) / 2;
    int box_y = row;

    WINDOW *box = newwin(box_h, box_w, box_y, box_x);
    wbkgd(box, COLOR_PAIR(CP_NORMAL));
    ui_box(box, CP_BORDER);

    mvwaddstr(box, 2, 4, "Hostname:");
    mvwaddstr(box, 4, 4, "SSH Key: ");

    /* Hostname field */
    if (focus == FLD_HOSTNAME) wattron(box, COLOR_PAIR(CP_SELECTED));
    mvwaddnstr(box, 2, 14, st->hostname[0] ? st->hostname : "(empty)", 40);
    if (focus == FLD_HOSTNAME) wattroff(box, COLOR_PAIR(CP_SELECTED));

    /* SSH Key row with Generate button */
    mvwaddstr(box, 4, 4, "SSH Key:");
    ui_button(box, 4, 14, "Generate New Key", focus == BTN_GEN_KEY);

    /* SSH Key display/edit field below button */
    if (focus == FLD_SSH_KEY) wattron(box, COLOR_PAIR(CP_SELECTED));
    if (st->ssh_key[0]) {
        mvwaddnstr(box, 6, 4, st->ssh_key, box_w - 8);
    } else {
        /* Blank field if no key, but still selectable */
        mvwhline(box, 6, 4, '_', box_w - 8);
    }
    if (focus == FLD_SSH_KEY) wattroff(box, COLOR_PAIR(CP_SELECTED));

    if (st->ssh_key[0]) {
        mvwaddstr(box, 8, 4, "Scan to copy SSH Key:");
        ui_draw_qr_code(box, 9, (box_w - 33) / 2, st->ssh_key);
    }

    wrefresh(box);
    delwin(box);

    row += box_h + 1;
    if (err_msg[0]) {
        ui_center(stdscr, row, err_msg, CP_DANGER, A_BOLD);
    }

    int btn_row = LINES - FOOTER_H - 2;
    ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == BTN_BACK);
    ui_button(stdscr, btn_row, bw - 16, "  Next →  ", focus == BTN_NEXT);

    refresh();
}

static bool auto_generate_ssh_key(char *key_out, int max_len)
{
    char tmp_key[] = "/tmp/barbarous_ssh_XXXXXX";
    int fd = mkstemp(tmp_key);
    if (fd < 0) return false;
    close(fd);
    remove(tmp_key); /* ssh-keygen wants to create the file itself */

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "ssh-keygen -t ed25519 -N \"\" -f %s -q", tmp_key);
    
    if (system(cmd) != 0) return false;

    char pub_path[512];
    snprintf(pub_path, sizeof(pub_path), "%s.pub", tmp_key);

    FILE *fp = fopen(pub_path, "r");
    bool ok = false;
    if (fp) {
        if (fgets(key_out, max_len, fp) != NULL) {
            /* Remove trailing newline if present */
            key_out[strcspn(key_out, "\r\n")] = '\0';
            ok = true;
        }
        fclose(fp);
    }

    /* Keep private key but remove pub file */
    remove(pub_path);

    return ok;
}

int screen_config(installer_state_t *st)
{
    int focus = FLD_HOSTNAME;
    char err_msg[128] = {0};
    
    const char *footer[] = {
        "TAB  Navigate",
        "ENTER Edit/Select",
        "← Back",
        "Q Quit",
    };

    while (1) {
        ui_draw_header("Step 2 of 6  —  Hostname and SSH key");
        ui_draw_footer(footer, 4);
        draw_config_screen(st, focus, err_msg);

        int ch = getch();
        err_msg[0] = '\0';

        switch (ch) {
            case KEY_UP: case 'k':
                focus = (focus - 1 + NUM_FOCUS) % NUM_FOCUS;
                break;
            case KEY_DOWN: case 'j': case '\t':
                focus = (focus + 1) % NUM_FOCUS;
                break;
            case KEY_LEFT:
                return NAV_PREV;
            case 'q': case 'Q':
                return NAV_QUIT;
            case '\n': case KEY_ENTER: case ' ':
                if (focus == FLD_HOSTNAME) {
                    WINDOW *box = newwin(10, 74, ui_body_top() + 7, (ui_body_width() - 74) / 2);
                    ui_readline(box, 2, 14, 40, st->hostname, MAX_HOSTNAME_LEN, false);
                    delwin(box);
                } else if (focus == BTN_GEN_KEY) {
                    ui_msgbox_timed("Please Wait", "Generating ED25519 keypair...", CP_ACCENT, 2);
                    if (auto_generate_ssh_key(st->ssh_key, MAX_SSH_LEN)) {
                        ui_alert("Success", "New key generated. PRIVATE KEY saved to /tmp/barbarous_ssh_*. Copy it before rebooting!", CP_SUCCESS);
                        focus = BTN_NEXT; /* Success -> Go to Next button */
                    } else {
                        ui_alert("Error", "Failed to run ssh-keygen.", CP_DANGER);
                    }
                } else if (focus == FLD_SSH_KEY) {
                    WINDOW *box = newwin(10, 74, ui_body_top() + 7, (ui_body_width() - 74) / 2);
                    ui_readline(box, 6, 4, 66, st->ssh_key, MAX_SSH_LEN, false);
                    delwin(box);
                } else if (focus == BTN_BACK) {
                    return NAV_PREV;
                } else if (focus == BTN_NEXT) {
                    if (st->hostname[0] == '\0') {
                        strncpy(err_msg, "Hostname cannot be empty.", sizeof(err_msg)-1);
                    } else if (st->ssh_key[0] == '\0') {
                        strncpy(err_msg, "SSH key is required for initial access.", sizeof(err_msg)-1);
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
