#ifndef NCURSES_H
#define NCURSES_H

#define CTRL_AND(c)  ((c) & 037)

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
int nc_getch(void);

#endif
