#include "ui.h"
#include "qrcodegen.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ─── Color initialisation ───────────────────────────────────────────────── */

void ui_init_colors(void)
{
    /* ncurses short-hand: use_default_colors() lets -1 = terminal default */
    use_default_colors();

    init_pair(CP_HEADER,     COLOR_WHITE,   COLOR_BLUE);
    init_pair(CP_FOOTER,     COLOR_BLACK,   COLOR_WHITE);
    init_pair(CP_NORMAL,     COLOR_WHITE,   -1);
    init_pair(CP_ACCENT,     COLOR_CYAN,    -1);
    init_pair(CP_KEY,        COLOR_YELLOW,  -1);
    init_pair(CP_SELECTED,   COLOR_BLACK,   COLOR_YELLOW);
    init_pair(CP_DANGER,     COLOR_RED,     -1);
    init_pair(CP_SUCCESS,    COLOR_GREEN,   -1);
    init_pair(CP_DIM,        COLOR_BLACK,   -1);   /* will be bold-black */
    init_pair(CP_BORDER,     COLOR_CYAN,    -1);
    init_pair(CP_BUTTON,     COLOR_BLACK,   COLOR_WHITE);
    init_pair(CP_BUTTON_FOC, COLOR_BLACK,   COLOR_YELLOW);
}

/* ─── Layout helpers ─────────────────────────────────────────────────────── */

int ui_body_top(void)    { return HEADER_H; }
int ui_body_height(void) { return LINES - HEADER_H - FOOTER_H; }
int ui_body_width(void)  { return COLS; }

void ui_clear_body(void)
{
    int top = ui_body_top();
    int bot = LINES - FOOTER_H;
    for (int r = top; r < bot; r++) {
        move(r, 0);
        clrtoeol();
    }
    refresh();
}

/* ─── Header ─────────────────────────────────────────────────────────────── */

void ui_draw_header(const char *title)
{
    int w = COLS;

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);

    /* Row 0 – fill */
    move(0, 0);
    for (int i = 0; i < w; i++) addch(' ');

    /* Row 1 – logo left, title centre, version right */
    move(1, 0);
    for (int i = 0; i < w; i++) addch(' ');

    const char *brand   = "  BARBAROUS CORE";
    const char *version = "v0.1  ";
    int tlen = (int)strlen(title);
    int vlen = (int)strlen(version);

    mvaddstr(1, 1,              brand);
    mvaddstr(1, (w - tlen) / 2, title);
    mvaddstr(1, w - vlen - 1,   version);

    /* Row 2 – separator line */
    move(2, 0);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    for (int i = 0; i < w; i++) addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);

    refresh();
}

/* ─── Footer ─────────────────────────────────────────────────────────────── */

void ui_draw_footer(const char * const *hints, int count)
{
    int w    = COLS;
    int row0 = LINES - FOOTER_H;

    /* Separator */
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    move(row0, 0);
    for (int i = 0; i < w; i++) addch(ACS_HLINE);
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);

    /* Key-hint row */
    attron(COLOR_PAIR(CP_FOOTER));
    move(row0 + 1, 0);
    for (int i = 0; i < w; i++) addch(' ');
    attroff(COLOR_PAIR(CP_FOOTER));

    /* Write each hint: "KEY" in bold, rest normal */
    int col = 2;
    for (int i = 0; i < count && col < w - 4; i++) {
        const char *h = hints[i];
        /* Split on first space to separate key from description */
        const char *sp = strchr(h, ' ');
        int klen = sp ? (int)(sp - h) : (int)strlen(h);

        attron(COLOR_PAIR(CP_BUTTON_FOC) | A_BOLD);
        mvaddnstr(row0 + 1, col, h, klen);
        attroff(COLOR_PAIR(CP_BUTTON_FOC) | A_BOLD);
        col += klen;

        if (sp) {
            attron(COLOR_PAIR(CP_FOOTER));
            mvaddstr(row0 + 1, col, sp);
            attroff(COLOR_PAIR(CP_FOOTER));
            col += (int)strlen(sp);
        }
        col += 3; /* gap between hints */
    }

    refresh();
}

/* ─── Box ────────────────────────────────────────────────────────────────── */

void ui_box(WINDOW *win, int pair)
{
    wattron(win, COLOR_PAIR(pair) | A_BOLD);
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(pair) | A_BOLD);
}

/* ─── Centred text ───────────────────────────────────────────────────────── */

void ui_center(WINDOW *win, int y, const char *text, int pair, attr_t attr)
{
    int w = getmaxx(win);
    int x = (w - (int)strlen(text)) / 2;
    if (x < 0) x = 0;
    wattron(win, COLOR_PAIR(pair) | attr);
    mvwaddstr(win, y, x, text);
    wattroff(win, COLOR_PAIR(pair) | attr);
}

/* ─── Button ─────────────────────────────────────────────────────────────── */

void ui_button(WINDOW *win, int y, int x, const char *label, bool focused)
{
    int pair = focused ? CP_BUTTON_FOC : CP_BUTTON;
    char buf[128];
    snprintf(buf, sizeof(buf), "  %s  ", label);
    wattron(win, COLOR_PAIR(pair) | A_BOLD);
    mvwaddstr(win, y, x, buf);
    wattroff(win, COLOR_PAIR(pair) | A_BOLD);
}

/* ─── Single-line input ──────────────────────────────────────────────────── */

int ui_readline(WINDOW *win, int y, int x, int w,
                char *buf, int bufsz, bool secret)
{
    int len = (int)strnlen(buf, bufsz - 1);
    curs_set(1);
    echo();

    while (1) {
        /* Redraw field */
        wattron(win, COLOR_PAIR(CP_SELECTED));
        wmove(win, y, x);
        for (int i = 0; i < w; i++) waddch(win, ' ');
        wmove(win, y, x);
        if (secret) {
            for (int i = 0; i < len; i++) waddch(win, '*');
        } else {
            waddnstr(win, buf, len);
        }
        wattroff(win, COLOR_PAIR(CP_SELECTED));
        wmove(win, y, x + len);
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == '\n' || ch == KEY_ENTER) break;
        if ((ch == KEY_BACKSPACE || ch == 127 || ch == '\b') && len > 0) {
            buf[--len] = '\0';
        } else if (ch >= 32 && ch < 127 && len < bufsz - 1 && len < w - 1) {
            buf[len++] = (char)ch;
            buf[len]   = '\0';
        }
    }

    noecho();
    curs_set(0);
    return len;
}

/* ─── Modal message box ──────────────────────────────────────────────────── */

void ui_msgbox(const char *title, const char *msg, int pair)
{
    int mw   = (int)strlen(msg) + 6;
    if ((int)strlen(title) + 6 > mw) mw = (int)strlen(title) + 6;
    if (mw > COLS - 4) mw = COLS - 4;
    int mh   = 7;
    int wy   = (LINES - mh) / 2;
    int wx   = (COLS  - mw) / 2;

    WINDOW *pop = newwin(mh, mw, wy, wx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));
    ui_box(pop, pair);
    ui_center(pop, 1, title, pair,    A_BOLD);
    mvwhline(pop, 2, 1, ACS_HLINE, mw - 2);
    ui_center(pop, 4, msg,   CP_NORMAL, 0);
    ui_center(pop, mh - 2, "[ Press any key ]", CP_DIM, A_BOLD);
    wrefresh(pop);
    wgetch(pop);
    delwin(pop);
    touchwin(stdscr);
    refresh();
}

/* ─── Yes / No confirmation dialog ──────────────────────────────────────── */

bool ui_confirm(const char *title, const char *msg)
{
    int mw = (int)strlen(msg) + 8;
    if ((int)strlen(title) + 8 > mw) mw = (int)strlen(title) + 8;
    if (mw > COLS - 4) mw = COLS - 4;
    int mh = 9;
    int wy = (LINES - mh) / 2;
    int wx = (COLS  - mw) / 2;

    WINDOW *pop = newwin(mh, mw, wy, wx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));
    keypad(pop, TRUE);

    bool choice = false;   /* false = No (safe default), true = Yes */

    while (1) {
        werase(pop);
        ui_box(pop, CP_DANGER);
        ui_center(pop, 1, title, CP_DANGER, A_BOLD);
        mvwhline(pop, 2, 1, ACS_HLINE, mw - 2);
        ui_center(pop, 4, msg, CP_NORMAL, 0);

        /* Two buttons centred on row 6 */
        int btn_y = 6;
        int yes_w = 9;
        int no_w  = 9;
        int gap   = 4;
        int total = yes_w + gap + no_w;
        int btn_x = (mw - total) / 2;
        if (btn_x < 1) btn_x = 1;

        wattron(pop, COLOR_PAIR(choice  ? CP_BUTTON_FOC : CP_BUTTON) | A_BOLD);
        mvwaddstr(pop, btn_y, btn_x,            "  Yes  ");
        wattroff(pop, COLOR_PAIR(choice  ? CP_BUTTON_FOC : CP_BUTTON) | A_BOLD);

        wattron(pop, COLOR_PAIR(!choice ? CP_BUTTON_FOC : CP_BUTTON) | A_BOLD);
        mvwaddstr(pop, btn_y, btn_x + yes_w + gap, "  No   ");
        wattroff(pop, COLOR_PAIR(!choice ? CP_BUTTON_FOC : CP_BUTTON) | A_BOLD);

        ui_center(pop, mh - 2,
                  "TAB switch    ENTER confirm    ESC cancel",
                  CP_DIM, A_BOLD);
        wrefresh(pop);

        int ch = wgetch(pop);
        switch (ch) {
            case '\t':              choice = !choice;   break;
            case KEY_LEFT:          choice = false;     break;
            case KEY_RIGHT:         choice = true;      break;
            case 'y': case 'Y':     choice = true;      goto done;
            case 'n': case 'N':     choice = false;     goto done;
            case '\n': case KEY_ENTER:                  goto done;
            case 27: case 'q': case 'Q':
                choice = false;                         goto done;
            default: break;
        }
    }
done:
    delwin(pop);
    touchwin(stdscr);
    refresh();
    return choice;
}

void ui_draw_qr_code(WINDOW *win, int y, int x, const char *text)
{
    if (!text || !text[0]) return;
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
                                   1, 40, qrcodegen_Mask_AUTO, true);
    if (!ok) return;
    int size = qrcodegen_getSize(qrcode);
    for (int dy = 0; dy < size; dy += 2) {
        for (int dx = 0; dx < size; dx++) {
            bool top = qrcodegen_getModule(qrcode, dx, dy);
            bool bot = (dy + 1 < size) ? qrcodegen_getModule(qrcode, dx, dy + 1) : false;
            if (top && bot) mvwaddstr(win, y + dy/2, x + dx, "█");
            else if (top) mvwaddstr(win, y + dy/2, x + dx, "▀");
            else if (bot) mvwaddstr(win, y + dy/2, x + dx, "▄");
            else mvwaddstr(win, y + dy/2, x + dx, " ");
        }
    }
}

int ui_menu(const char *title, const char **items, int count, int initial)
{
    int mw = 40;
    for (int i = 0; i < count; i++) {
        int l = (int)strlen(items[i]);
        if (l + 6 > mw) mw = l + 6;
    }
    if (mw > COLS - 4) mw = COLS - 4;

    int mh = 15;
    if (count + 6 < mh) mh = count + 6;
    if (mh > LINES - 4) mh = LINES - 4;

    int wy = (LINES - mh) / 2;
    int wx = (COLS - mw) / 2;

    WINDOW *pop = newwin(mh, mw, wy, wx);
    wbkgd(pop, COLOR_PAIR(CP_NORMAL));
    keypad(pop, TRUE);

    int selected = (initial >= 0 && initial < count) ? initial : 0;
    int scroll = 0;
    int visible = mh - 4;

    while (1) {
        werase(pop);
        ui_box(pop, CP_ACCENT);
        ui_center(pop, 1, title, CP_ACCENT, A_BOLD);
        mvwhline(pop, 2, 1, ACS_HLINE, mw - 2);

        for (int i = 0; i < visible && (i + scroll) < count; i++) {
            int idx = i + scroll;
            bool sel = (idx == selected);
            if (sel) wattron(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            
            /* Clear row */
            mvwhline(pop, 3 + i, 2, ' ', mw - 4);
            mvwprintw(pop, 3 + i, 4, "%s", items[idx]);
            
            if (sel) wattroff(pop, COLOR_PAIR(CP_SELECTED) | A_BOLD);
        }

        /* Scroll indicators */
        if (scroll > 0) mvwaddch(pop, 3, mw - 2, ACS_UARROW);
        if (scroll + visible < count) mvwaddch(pop, mh - 2, mw - 2, ACS_DARROW);

        wrefresh(pop);

        int ch = wgetch(pop);
        switch (ch) {
            case KEY_UP: case 'k':
                if (selected > 0) {
                    selected--;
                    if (selected < scroll) scroll = selected;
                }
                break;
            case KEY_DOWN: case 'j':
                if (selected < count - 1) {
                    selected++;
                    if (selected >= scroll + visible) scroll = selected - visible + 1;
                }
                break;
            case '\n': case KEY_ENTER: case ' ':
                delwin(pop);
                touchwin(stdscr);
                refresh();
                return selected;
            case 27: case 'q': case 'Q':
                delwin(pop);
                touchwin(stdscr);
                refresh();
                return -1;
            default: break;
        }
    }
}

void ui_progress_bar(WINDOW *win, int y, int x, int w, int percent, int pair)
{
    if (w < 5) return;
    mvwaddch(win, y, x, '[');
    mvwaddch(win, y, x + w - 1, ']');
    
    int bar_w = w - 2;
    int filled = (percent * bar_w) / 100;
    
    wattron(win, COLOR_PAIR(pair) | A_BOLD);
    for (int i = 0; i < filled; i++) {
        mvwaddch(win, y, x + 1 + i, ACS_CKBOARD);
    }
    wattroff(win, COLOR_PAIR(pair) | A_BOLD);
    
    for (int i = filled; i < bar_w; i++) {
        mvwaddch(win, y, x + 1 + i, ' ');
    }
    
    char buf[16];
    snprintf(buf, sizeof(buf), " %3d%% ", percent);
    mvwaddstr(win, y, x + w + 1, buf);
}

