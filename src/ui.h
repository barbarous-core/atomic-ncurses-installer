#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <stdbool.h>

/* ─── Color pair IDs ─────────────────────────────────────────────────────── */
#define CP_HEADER      1   /* white   on dark-blue  – top bar              */
#define CP_FOOTER      2   /* black   on white      – bottom bar           */
#define CP_NORMAL      3   /* white   on black      – body text            */
#define CP_ACCENT      4   /* cyan    on black      – highlights / titles  */
#define CP_KEY         5   /* yellow  on black      – keyboard hints       */
#define CP_SELECTED    6   /* black   on cyan       – selected item        */
#define CP_DANGER      7   /* red     on black      – warnings / errors    */
#define CP_SUCCESS     8   /* green   on black      – success messages     */
#define CP_DIM         9   /* dark-grey on black    – inactive / dim text  */
#define CP_BORDER      10  /* cyan    on black      – box borders          */
#define CP_BUTTON      11  /* black   on white      – unfocused button     */
#define CP_BUTTON_FOC  12  /* black   on yellow     – focused button       */

/* ─── Layout constants ───────────────────────────────────────────────────── */
#define HEADER_H   3
#define FOOTER_H   2

/* ─── Public API ─────────────────────────────────────────────────────────── */

/* Must be called once after start_color() */
void ui_init_colors(void);

/* Draw the persistent top bar (title = current step label) */
void ui_draw_header(const char *title);

/* Draw the persistent bottom bar with key hints
 * hints[]  = array of "KEY  Description" strings
 * count    = number of hints
 */
void ui_draw_footer(const char * const *hints, int count);

/* Draw a rounded box in window win */
void ui_box(WINDOW *win, int pair);

/* Write text centred on row y inside win, using color pair */
void ui_center(WINDOW *win, int y, const char *text, int pair, attr_t attr);

/* Draw a focusable button; returns its column start */
void ui_button(WINDOW *win, int y, int x, const char *label, bool focused);

/* Prompt for single-line text input inside win at (y,x), width w.
 * secret=true masks input as '*'.
 * Returns number of characters entered.
 */
int ui_readline(WINDOW *win, int y, int x, int w,
                char *buf, int bufsz, bool secret);

/* Show a modal message box; press any key to dismiss */
void ui_msgbox(const char *title, const char *msg, int pair);

/* Yes/No confirmation dialog.
 * Returns true if user pressed Y/ENTER, false for N/ESC/Q. */
bool ui_confirm(const char *title, const char *msg);

/* Clear the body area (between header and footer) */
void ui_clear_body(void);

/* Return the usable body height / width */
int ui_body_height(void);
int ui_body_width(void);
/* First row of the body (after header) */
int ui_body_top(void);

/* Draw a QR code for the given text in window win at (y,x) */
void ui_draw_qr_code(WINDOW *win, int y, int x, const char *text);

/* Show a scrollable menu and return the index of the selected item, or -1 if cancelled */
int ui_menu(const char *title, const char **items, int count, int initial);

#endif /* UI_H */
