#ifndef NCURSES_H
#define NCURSES_H

#define CTRL_AND(c)  ((c) & 037)

#include <stdbool.h>

void nc_init(void);
void nc_term(void);
void nc_clearall(void);

void nc_addch(char);
void nc_addstr(char *);

void nc_highlight(int on);

void nc_vstatus(const char *fmt, va_list l, int right);
void nc_get_yx(int *y, int *x);
void nc_set_yx(int y, int x);

int nc_LINES(void);
int nc_COLS(void);
void nc_clrtoeol(void);

/* called by io functions */
int nc_getch(bool mapraw, bool *wasraw);
/* mapraw = convert ^Vx to escape char */

int nc_charlen(int ch);

enum nc_style
{
	ATTR_BOLD = 1 << 0,
	COL_BLUE = 1 << 1,
	COL_BROWN = 1 << 2,
};
void nc_style(enum nc_style);

#endif
