#ifndef NCURSES_H
#define NCURSES_H

void nc_init(void);
void nc_term(void);
void nc_cls(void);

int nc_getch(void);
void nc_addch(char);
void nc_addstr(char *);

void nc_vstatus(const char *fmt, va_list l);
void nc_get_yx(int *y, int *x);
void nc_set_yx(int y, int x);

int nc_LINES(void);
void nc_clrtoeol(void);

#endif
