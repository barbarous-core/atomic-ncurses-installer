#include "generate.h"
#include "../ui.h"
#include "../installer.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void draw_summary(const installer_state_t *st, bool done, bool success, const char *msg)
{
    ui_clear_body();
    
    int bw = ui_body_width();
    int top = ui_body_top();
    int row = top + 1;
    
    ui_center(stdscr, row, "Installation Summary", CP_ACCENT, A_BOLD);
    row += 2;
    
    int col1 = (bw / 2) - 25;
    if (col1 < 2) col1 = 2;

    attron(COLOR_PAIR(CP_NORMAL));
    mvprintw(row++, col1, "Target Edition:   %s", st->edition);
    mvprintw(row++, col1, "System Hostname:  %s", st->hostname);
    mvprintw(row++, col1, "SSH Public Key:   %s", st->ssh_key[0] ? "[Provided]" : "[None]");
    mvprintw(row++, col1, "Target Disk:      %s", st->disk[0] ? st->disk : "None");
    row++;
    mvprintw(row++, col1, "Ignition File:    %s", st->output_path);
    attroff(COLOR_PAIR(CP_NORMAL));

    /* QR Code on the right side of the summary */
    if (st->ssh_key[0]) {
        int qr_col = bw / 2 + 5;
        if (qr_col + 35 < bw) { /* only if it fits */
            attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
            mvaddstr(top + 3, qr_col, "Scan SSH Key:");
            attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
            ui_draw_qr_code(stdscr, top + 4, qr_col, st->ssh_key);
        }
    }

    row += 2;

    if (done) {
        if (success) {
            ui_center(stdscr, row, "✅ Installation starting / Ignition written!", CP_SUCCESS, A_BOLD);
            row++;
            ui_center(stdscr, row, msg, CP_NORMAL, 0);
        } else {
            ui_center(stdscr, row, "❌ Error during installation/generation!", CP_DANGER, A_BOLD);
            row++;
            ui_center(stdscr, row, msg, CP_DANGER, 0);
        }
    } else {
        ui_center(stdscr, row, "Press ENTER to generate Ignition and start installation.", CP_KEY, A_BOLD);
    }

    int btn_row = LINES - FOOTER_H - 2;
    if (done) {
        ui_button(stdscr, btn_row, (bw - 12)/2, "  Quit  ", true);
    } else {
        ui_button(stdscr, btn_row, 3,       "  ← Back  ", false);
        ui_button(stdscr, btn_row, bw - 18, "  Install  ", true);
    }

    refresh();
}

static bool generate_ignition(const installer_state_t *st)
{
    FILE *f = fopen(st->output_path, "w");
    if (!f) return false;

    fprintf(f, "{\n");
    fprintf(f, "  \"ignition\": { \"version\": \"3.3.0\" },\n");
    
    /* storage: hostname and basic files */
    fprintf(f, "  \"storage\": {\n");
    fprintf(f, "    \"files\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"path\": \"/etc/hostname\",\n");
    fprintf(f, "        \"contents\": { \"source\": \"data:,%s%%0A\" },\n", st->hostname);
    fprintf(f, "        \"mode\": 420\n");
    fprintf(f, "      }\n");
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");

    /* passwd: core user with SSH key */
    fprintf(f, "  \"passwd\": {\n");
    fprintf(f, "    \"users\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"name\": \"core\",\n");
    fprintf(f, "        \"sshAuthorizedKeys\": [ \"%s\" ],\n", st->ssh_key);
    fprintf(f, "        \"groups\": [ \"wheel\", \"sudo\" ]\n");
    fprintf(f, "      }\n");
    fprintf(f, "    ]\n");
    fprintf(f, "  },\n");

    /* systemd units for rpm-ostree and assets */
    fprintf(f, "  \"systemd\": {\n");
    fprintf(f, "    \"units\": [\n");
    fprintf(f, "      {\n");
    fprintf(f, "        \"name\": \"install-assets.service\",\n");
    fprintf(f, "        \"enabled\": true,\n");
    fprintf(f, "        \"contents\": \"[Unit]\\nDescription=Install Local Assets\\nAfter=local-fs.target\\n[Service]\\nType=oneshot\\nExecStart=/usr/bin/echo Installing assets...\\n[Install]\\nWantedBy=multi-user.target\"\n");
    fprintf(f, "      }\n");
    fprintf(f, "    ]\n");
    fprintf(f, "  }\n");

    fprintf(f, "}\n");
    fclose(f);
    return true;
}

int screen_generate(installer_state_t *st)
{
    const char *footer[] = {
        "ENTER Confirm",
        "← Back",
        "Q Quit",
    };
    
    bool done = false;
    bool success = false;
    char msg[512] = {0};
    
    while (1) {
        ui_draw_header("Step 5 of 5  —  Summary & Installation");
        ui_draw_footer(footer, 3);
        draw_summary(st, done, success, msg);

        int ch = getch();

        switch (ch) {
            case KEY_LEFT: case 'h':
                if (!done) return NAV_PREV;
                break;
            case '\n': case KEY_ENTER: case ' ':
                if (done) {
                    return NAV_QUIT;
                } else {
                    success = generate_ignition(st);
                    if (success) {
                        snprintf(msg, sizeof(msg), "Saved to %s", st->output_path);
                    } else {
                        snprintf(msg, sizeof(msg), "Failed to open output path %s for writing.", st->output_path);
                    }
                    done = true;
                }
                break;
            case 'q': case 'Q':
                return NAV_QUIT;
            case KEY_RESIZE:
                break;
            default:
                break;
        }
    }
}
