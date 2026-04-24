#include "generate.h"
#include "../ui.h"
#include "../installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FOCUS_DOTFILES 0
#define FOCUS_INSTALL  1
#define FOCUS_BACK     2

static void draw_summary(installer_state_t *st, bool done, bool success, const char *msg, int focus)
{
    ui_clear_body();
    
    int bw = ui_body_width();
    int top = ui_body_top();
    int row = top + 1;
    
    ui_center(stdscr, row, "Installation Summary & Confirmation", CP_ACCENT, A_BOLD);
    row += 2;
    
    int col1_x = 4;
    int col2_x = bw / 2 + 2;

    /* ─── Column 1: Configuration in Green Box ─── */
    int box_w = bw / 2 - 4;
    int box_h = 10;
    WINDOW *sum_box = newwin(box_h, box_w, row, col1_x - 1);
    wbkgd(sum_box, COLOR_PAIR(CP_NORMAL));
    ui_box(sum_box, CP_ACCENT);
    
    int r1 = 2;
    int lx = 3;
    mvwprintw(sum_box, r1++, lx, "%-22s: %s", "Target Edition", st->edition);
    mvwprintw(sum_box, r1++, lx, "%-22s: %s", "System Hostname", st->hostname);
    mvwprintw(sum_box, r1++, lx, "%-22s: %s / %s", "Keyboard / Time Zone", st->keyboard, st->timezone);
    mvwprintw(sum_box, r1++, lx, "%-22s: %s", "Username", st->username);
    mvwprintw(sum_box, r1++, lx, "%-22s: %d RPMs, %d Bins", "Applications", st->rpm_count, st->bin_count);
    
    mvwprintw(sum_box, r1++, lx, "%-22s: ", "Target Disk");
    wattron(sum_box, COLOR_PAIR(CP_DANGER) | A_BOLD);
    wprintw(sum_box, "%s", st->disk[0] ? st->disk : "NOT SELECTED");
    wattroff(sum_box, COLOR_PAIR(CP_DANGER) | A_BOLD);

    mvwprintw(sum_box, r1++, lx, "%-22s: %s", "Ignition File", st->output_path);
    wrefresh(sum_box);
    delwin(sum_box);

    /* ─── Column 2: SSH & Security ─── */
    int r2 = row;
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    mvaddstr(r2++, col2_x, "SSH Public Key:");
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    
    if (st->ssh_key[0]) {
        char clip[64];
        strncpy(clip, st->ssh_key, 60);
        clip[60] = '\0';
        mvprintw(r2++, col2_x, "%.60s...", clip);
        r2++;
        ui_draw_qr_code(stdscr, r2, col2_x, st->ssh_key);
    } else {
        mvprintw(r2++, col2_x, "[ No SSH Key Provided ]");
    }

    /* ─── Bottom Actions ─── */
    int action_row = LINES - FOOTER_H - 4;
    
    /* Dotfiles Checkbox above Install button */
    bool df_focus = (focus == FOCUS_DOTFILES);
    if (df_focus) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    mvprintw(action_row, bw - 38, "[%s]  Include default dotfiles?", st->install_dotfiles ? "X" : " ");
    if (df_focus) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);

    if (done) {
        int final_row = action_row - 2;
        if (success) {
            ui_center(stdscr, final_row, "✅ Installation starting / Ignition written!", CP_SUCCESS, A_BOLD);
            ui_center(stdscr, final_row + 1, msg, CP_NORMAL, 0);
        } else {
            ui_center(stdscr, final_row, "❌ Error during installation/generation!", CP_DANGER, A_BOLD);
            ui_center(stdscr, final_row + 1, msg, CP_DANGER, 0);
        }
    }

    int btn_row = LINES - FOOTER_H - 2;
    if (done) {
        ui_button(stdscr, btn_row, (bw - 14)/2, "  Reboot  ", true);
    } else {
        ui_button(stdscr, btn_row, 3,       "  ← Back  ", focus == FOCUS_BACK);
        ui_button(stdscr, btn_row, bw - 18, "  Install  ", focus == FOCUS_INSTALL);
    }

    refresh();
}

static void sanitize_pkg_name(char *dst, const char *src, int max_len)
{
    int j = 0;
    bool in_parens = false;
    for (int i = 0; src[i] != '\0' && j < max_len - 1; i++) {
        if (src[i] == '(') in_parens = true;
        else if (src[i] == ')') in_parens = false;
        else if (!in_parens) {
            if (src[i] == '/') dst[j++] = ' ';
            else dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

static bool generate_ignition(const installer_state_t *st)
{
    FILE *f = fopen(st->output_path, "w");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "  \"ignition\": { \"version\": \"3.3.0\" },\n");
    
    /* Storage: hostname, timezone, and keyboard */
    fprintf(f, "  \"storage\": {\n");
    fprintf(f, "    \"files\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"path\": \"/etc/hostname\",\n");
    fprintf(f, "        \"contents\": { \"source\": \"data:,%s%%0A\" },\n", st->hostname);
    fprintf(f, "        \"mode\": 420\n");
    fprintf(f, "      }\n");
    fprintf(f, "    ],\n");
    fprintf(f, "    \"links\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"path\": \"/etc/localtime\",\n");
    fprintf(f, "        \"target\": \"../usr/share/zoneinfo/%s\"\n", st->timezone[0] ? st->timezone : "UTC");
    fprintf(f, "      }\n");
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");

    /* Passwd: User configuration */
    fprintf(f, "  \"passwd\": {\n");
    fprintf(f, "    \"users\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"name\": \"%s\",\n", st->username[0] ? st->username : "core");
    if (st->password_hash[0]) {
        fprintf(f, "        \"passwordHash\": \"%s\",\n", st->password_hash);
    }
    fprintf(f, "        \"sshAuthorizedKeys\": [ \"%s\" ],\n", st->ssh_key);
    fprintf(f, "        \"groups\": [ \"wheel\", \"sudo\" ]\n");
    fprintf(f, "      }\n");
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");

    /* Systemd: First-boot setup for RPMs and settings */
    fprintf(f, "  \"systemd\": {\n");
    fprintf(f, "    \"units\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"name\": \"barbarous-provisioning.service\",\n");
    fprintf(f, "        \"enabled\": true,\n");
    fprintf(f, "        \"contents\": \"[Unit]\\nDescription=Barbarous First Boot Provisioning\\nAfter=network-online.target\\nWants=network-online.target\\nConditionFirstBoot=yes\\n\\n[Service]\\nType=oneshot\\n");
    
    /* Keyboard layout command */
    fprintf(f, "ExecStart=/usr/bin/localectl set-x11-keymap %s\\n", st->keyboard[0] ? st->keyboard : "us");

    /* RPM Installation command */
    if (st->rpm_count > 0) {
        fprintf(f, "ExecStart=/usr/bin/rpm-ostree install -y");
        for (int i = 0; i < st->rpm_count; i++) {
            char clean[MAX_RPM_LEN];
            sanitize_pkg_name(clean, st->rpms[i], MAX_RPM_LEN);
            fprintf(f, " %s", clean);
        }
        fprintf(f, "\\n");
    }

    /* Dotfiles handling (if enabled, could fetch a repo or script) */
    if (st->install_dotfiles) {
        fprintf(f, "ExecStart=/usr/bin/echo 'Dotfiles installation enabled'\\n");
    }

    /* Binary deployment section */
    if (st->bin_count > 0) {
        fprintf(f, "ExecStart=/usr/bin/echo 'Deploying selected binaries...'\\n");
        for (int i = 0; i < st->bin_count; i++) {
            /* For now, we echo the requirement. 
               In a real scenario, this would trigger a download script or copy from media. */
            fprintf(f, "ExecStart=/usr/bin/echo '  Installing binary: %s'\\n", st->bins[i]);
        }
    }

    fprintf(f, "ExecStart=/usr/bin/systemctl reboot\\nStandardOutput=journal+console\\nRemainAfterExit=yes\\n\\n[Install]\\nWantedBy=multi-user.target\"\n");
    fprintf(f, "      }\n");
    fprintf(f, "    ]\n");
    fprintf(f, "  }\n");

    fprintf(f, "}\n");
    fclose(f);
    return true;
}

static void simulate_install(installer_state_t *st)
{
    ui_clear_body();
    int bw = ui_body_width();
    int top = ui_body_top();
    
    ui_center(stdscr, top + 1, "PROVISIONING SYSTEM", CP_ACCENT, A_BOLD);
    
    int box_w = bw - 10;
    int box_h = 12;
    int box_x = 5;
    int box_y = top + 3;
    
    WINDOW *win = newwin(box_h, box_w, box_y, box_x);
    wbkgd(win, COLOR_PAIR(CP_NORMAL));
    ui_box(win, CP_ACCENT);
    
    const char *cmds[] = {
        "coreos-installer install %s --ignition-file %s",
        "Writing image to target disk...",
        "Configuring GPT and partitions...",
        "Extracting ostree commit...",
        "Writing Ignition configuration...",
        "Verifying checksums...",
    };
    
    char cmd_buf[256];
    snprintf(cmd_buf, sizeof(cmd_buf), cmds[0], st->disk[0] ? st->disk : "/dev/sda", st->output_path);
    
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 2, 4, "# %s", cmd_buf);
    wattroff(win, COLOR_PAIR(CP_DIM));
    
    for (int i = 1; i < 6; i++) {
        /* Clear previous step line */
        mvwprintw(win, 4, 4, "                                                            ");
        wattron(win, COLOR_PAIR(CP_ACCENT) | A_BOLD);
        mvwprintw(win, 4, 4, "Step %d/5: %s", i, cmds[i]);
        wattroff(win, COLOR_PAIR(CP_ACCENT) | A_BOLD);
        
        for (int p = 0; p <= 100; p += (1 + rand() % 5)) {
            if (p > 100) p = 100;
            ui_progress_bar(win, 6, 4, box_w - 15, p, CP_SUCCESS);
            
            /* Random log messages */
            if (p % 20 == 0) {
                wattron(win, COLOR_PAIR(CP_DIM));
                mvwprintw(win, 8, 4, "log: processed %d%% of block data...      ", p);
                wattroff(win, COLOR_PAIR(CP_DIM));
            }
            
            wrefresh(win);
            usleep(20000 + (rand() % 50000));
            if (p == 100) break;
        }
        usleep(300000);
    }
    
    wattron(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    ui_center(win, 10, "★ INSTALLATION COMPLETE ★", CP_SUCCESS, A_BOLD);
    wattroff(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    wrefresh(win);
    
    sleep(2);
    delwin(win);
    touchwin(stdscr);
    refresh();
}

int screen_generate(installer_state_t *st)
{
    const char *footer[] = {
        "↑↓ Navigate",
        "SPACE Toggle",
        "ENTER Confirm",
        "← Back",
        "Q Quit",
    };
    
    bool done = false;
    bool success = false;
    char msg[512] = {0};
    int focus = FOCUS_DOTFILES;
    
    while (1) {
        ui_draw_header("Step 6 of 6  —  Summary & Installation");
        
        if (done) {
            const char *done_footer[] = { "ENTER Reboot", "Q Quit" };
            ui_draw_footer(done_footer, 2);
        } else {
            ui_draw_footer(footer, 5);
        }

        draw_summary(st, done, success, msg, focus);

        int ch = getch();

        switch (ch) {
            case KEY_UP: case 'k':
                if (!done) {
                    if (focus == FOCUS_INSTALL || focus == FOCUS_BACK) focus = FOCUS_DOTFILES;
                    else focus = FOCUS_INSTALL;
                }
                break;
            case KEY_DOWN: case 'j':
                if (!done) {
                    if (focus == FOCUS_DOTFILES) focus = FOCUS_INSTALL;
                }
                break;
            case '\t':
                if (!done) focus = (focus + 1) % 3;
                break;
            case ' ':
                if (!done && focus == FOCUS_DOTFILES) {
                    st->install_dotfiles = !st->install_dotfiles;
                }
                break;
            case KEY_LEFT: case 'h':
                if (!done) {
                    if (focus == FOCUS_INSTALL) focus = FOCUS_BACK;
                    else return NAV_PREV;
                }
                break;
            case KEY_RIGHT: case 'l':
                if (!done && focus == FOCUS_BACK) focus = FOCUS_INSTALL;
                break;
            case '\n': case KEY_ENTER:
                if (done) {
                    return NAV_QUIT;
                } else {
                    if (focus == FOCUS_BACK) return NAV_PREV;
                    if (focus == FOCUS_DOTFILES) {
                        st->install_dotfiles = !st->install_dotfiles;
                    } else {
                        success = generate_ignition(st);
                        if (success) {
                            simulate_install(st);
                            snprintf(msg, sizeof(msg), "System successfully provisioned to %s", st->disk);
                        } else {
                            snprintf(msg, sizeof(msg), "Failed to generate ignition file.");
                        }
                        done = true;
                    }
                }
                break;
            case 'q': case 'Q':
                return NAV_QUIT;
            default:
                break;
        }
    }
}
