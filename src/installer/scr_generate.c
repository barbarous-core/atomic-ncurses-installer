#include "scr_generate.h"
#include "../common/ui.h"


#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

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

static void base64_encode_binary(const unsigned char *src, int len, char *dst) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    for (; i < len; i += 3) {
        unsigned int val = src[i] << 16;
        if (i + 1 < len) val |= src[i + 1] << 8;
        if (i + 2 < len) val |= src[i + 2];

        dst[j++] = table[(val >> 18) & 0x3F];
        dst[j++] = table[(val >> 12) & 0x3F];
        dst[j++] = (i + 1 < len) ? table[(val >> 6) & 0x3F] : '=';
        dst[j++] = (i + 2 < len) ? table[val & 0x3F] : '=';
    }
    dst[j] = '\0';
}


static void write_custom_matrix_json(FILE *f, const installer_state_t *st) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);

    char path[512];
    snprintf(path, sizeof(path), "/home/%s/.barbarous/matrix_%s_%s.csv", 
             st->username[0] ? st->username : "core", 
             st->username[0] ? st->username : "core",
             date_str);

    char csv_data[8192] = {0};
    strcat(csv_data, "Category,Name,Type\n");
    
    for (int i = 0; i < st->rpm_count; i++) {
        char line[512];
        snprintf(line, sizeof(line), "Selected,%s,rpm\n", st->rpms[i]);
        strcat(csv_data, line);
    }
    for (int i = 0; i < st->bin_count; i++) {
        char line[512];
        snprintf(line, sizeof(line), "Selected,%s,bin\n", st->bins[i]);
        strcat(csv_data, line);
    }

    char b64_data[12288] = {0};
    base64_encode_binary((const unsigned char*)csv_data, strlen(csv_data), b64_data);

    fprintf(f, "      {\n");
    fprintf(f, "        \"path\": \"%s\",\n", path);
    fprintf(f, "        \"contents\": { \"source\": \"data:;base64,%s\" },\n", b64_data);
    fprintf(f, "        \"mode\": 420,\n");
    fprintf(f, "        \"overwrite\": true\n");
    fprintf(f, "      }\n");
}

static void save_local_matrix_copy(const installer_state_t *st) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);

    char filename[512];
    snprintf(filename, sizeof(filename), "matrix_%s_%s.csv", 
             st->username[0] ? st->username : "core", date_str);

    FILE *f = fopen(filename, "w");
    if (f) {
        fprintf(f, "Category,Name,Type\n");
        for (int i = 0; i < st->rpm_count; i++) {
            fprintf(f, "Selected,%s,rpm\n", st->rpms[i]);
        }
        for (int i = 0; i < st->bin_count; i++) {
            fprintf(f, "Selected,%s,bin\n", st->bins[i]);
        }
        fclose(f);
    }
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
    fprintf(f, "      },\n");
    write_custom_matrix_json(f, st);
    fprintf(f, ",\n");

    /* Embed the compiled barbarous-setup-tui binary */
    FILE *bin_fp = NULL;
    const char *search_paths[] = {
        "barbarous-setup-tui",
        "/usr/local/bin/barbarous-setup-tui",
        "/run/media/iso/barbarous-assets/bin/barbarous-setup-tui",
        "/mnt/barbarous-assets/bin/barbarous-setup-tui",
        "/run/media/liveuser/barbarous-assets/bin/barbarous-setup-tui",
        NULL
    };

    for (int i = 0; search_paths[i] != NULL; i++) {
        bin_fp = fopen(search_paths[i], "rb");
        if (bin_fp) break;
    }
    
    if (!bin_fp) {
        char exe_path[1024];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len > 0) {
            exe_path[len] = '\0';
            char *last_slash = strrchr(exe_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                char setup_path[1024];
                snprintf(setup_path, sizeof(setup_path), "%s/barbarous-setup-tui", exe_path);
                bin_fp = fopen(setup_path, "rb");
            }
        }
    }
    
    if (bin_fp) {
        fseek(bin_fp, 0, SEEK_END);
        long bin_size = ftell(bin_fp);
        fseek(bin_fp, 0, SEEK_SET);

        unsigned char *bin_data = malloc(bin_size);
        if (bin_data) {
            fread(bin_data, 1, bin_size, bin_fp);
            
            long b64_len = (bin_size / 3 + 1) * 4 + 1;
            char *b64_data = malloc(b64_len);
            
            if (b64_data) {
                base64_encode_binary(bin_data, bin_size, b64_data);
                
                fprintf(f, "      {\n");
                fprintf(f, "        \"path\": \"/usr/local/bin/barbarous-setup-tui\",\n");
                fprintf(f, "        \"contents\": { \"source\": \"data:;base64,%s\" },\n", b64_data);
                fprintf(f, "        \"mode\": 493\n"); // 0755
                fprintf(f, "      }\n");
                
                free(b64_data);
            } else {
                fclose(bin_fp);
                free(bin_data);
                fclose(f);
                return false;
            }
            free(bin_data);
        } else {
            fclose(bin_fp);
            fclose(f);
            return false;
        }
        fclose(bin_fp);
    } else {
        // Could not find binary
        ui_msgbox("Error", "Could not find barbarous-setup-tui! (searched ISO assets, /usr/local/bin, and local)", CP_DANGER);
        fclose(f);
        return false;
    }
    fprintf(f, "    ],\n");
    fprintf(f, "    \"directories\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"path\": \"/home/%s/.barbarous\",\n", st->username[0] ? st->username : "core");
    fprintf(f, "        \"mode\": 448,\n");
    fprintf(f, "        \"user\": { \"name\": \"%s\" },\n", st->username[0] ? st->username : "core");
    fprintf(f, "        \"group\": { \"name\": \"%s\" }\n", st->username[0] ? st->username : "core");
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
    fprintf(f, "  }\n");

    fprintf(f, "}\n");
    fclose(f);
    save_local_matrix_copy(st);
    return true;
}

static void run_real_install(installer_state_t *st)
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
    
    char cmd[512];
    /* Note: coreos-installer usually needs root. 
       We assume the TUI is run with necessary privileges. */
    snprintf(cmd, sizeof(cmd), "coreos-installer install %s --ignition-file %s 2>&1", 
             st->disk[0] ? st->disk : "/dev/sda", st->output_path);
    
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 2, 4, "# %s", cmd);
    wattroff(win, COLOR_PAIR(CP_DIM));
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        ui_center(win, 6, "Error: Could not execute coreos-installer", CP_DANGER, A_BOLD);
        wrefresh(win);
        sleep(2);
        delwin(win);
        return;
    }

    char line[512];
    int p = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Clean the line of trailing whitespace/newlines */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        /* Parse percentage if present (e.g. "... 45% ...") */
        char *pct_ptr = strchr(line, '%');
        if (pct_ptr && pct_ptr > line) {
            char *start = pct_ptr - 1;
            while (start > line && isdigit((unsigned char)line[(start - line) - 1])) start--;
            int val = atoi(start);
            if (val >= 0 && val <= 100) p = val;
        }
        
        /* Display current status line */
        mvwprintw(win, 4, 4, "                                                                ");
        if (strlen(line) > (size_t)box_w - 10) line[box_w - 10] = '\0';
        mvwprintw(win, 4, 4, "Status: %s", line);
        
        /* Update progress bar */
        ui_progress_bar(win, 6, 4, box_w - 15, p, CP_SUCCESS);
        
        wrefresh(win);
    }
    
    int status = pclose(fp);
    
    if (status == 0) {
        /* Final Instructions for the new strategy */
        char instructions[512];
        snprintf(instructions, sizeof(instructions), 
                 "Base system is installed!\n\n"
                 "1. Reboot and login as '%s'\n"
                 "2. Insert the Barbarous ISO\n"
                 "3. Run: sudo barbarous-setup-tui\n\n"
                 "This will install all your selected RPMs and Binaries.",
                 st->username[0] ? st->username : "core");
        ui_alert("Next Steps", instructions, CP_ACCENT);

        wattron(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        ui_center(win, 10, "★ INSTALLATION COMPLETE ★", CP_SUCCESS, A_BOLD);
        wattroff(win, COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    } else {
        wattron(win, COLOR_PAIR(CP_DANGER) | A_BOLD);
        ui_center(win, 10, "✖ INSTALLATION FAILED ✖", CP_DANGER, A_BOLD);
        wattroff(win, COLOR_PAIR(CP_DANGER) | A_BOLD);
    }
    
    wrefresh(win);
    sleep(3);
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
                        char confirm_msg[256];
                        snprintf(confirm_msg, sizeof(confirm_msg), 
                                 "Are you sure? This will PERMANENTLY WIPE %s.", 
                                 st->disk[0] ? st->disk : "/dev/sda");
                        
                        if (!ui_confirm("⚠️  DESTRUCTIVE ACTION ⚠️", confirm_msg)) {
                            break;
                        }

                        success = generate_ignition(st);
                        if (success) {
                            run_real_install(st);
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
