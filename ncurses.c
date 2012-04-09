#include <ncurses.h>

#include "ncurses.h"

void nc_init()
{
	initscr();
	noecho();
	cbreak();
	raw();
	scrollok(stdscr, 1);

	nonl();
	intrflush(stdscr, 0);

	if(has_colors()){
		start_color();
		use_default_colors();

		init_pair(1 + COLOR_BLACK,   COLOR_BLACK,   -1);
		init_pair(1 + COLOR_GREEN,   COLOR_GREEN,   -1);
		init_pair(1 + COLOR_WHITE,   COLOR_WHITE,   -1);
		init_pair(1 + COLOR_RED,     COLOR_RED,     -1);
		init_pair(1 + COLOR_CYAN,    COLOR_CYAN,    -1);
		init_pair(1 + COLOR_MAGENTA, COLOR_MAGENTA, -1);
		init_pair(1 + COLOR_BLUE,    COLOR_BLUE,    -1);
		init_pair(1 + COLOR_YELLOW,  COLOR_YELLOW,  -1);

		init_pair(9 + COLOR_BLACK,   -1, COLOR_BLACK);
		init_pair(9 + COLOR_GREEN,   -1, COLOR_GREEN);
		init_pair(9 + COLOR_WHITE,   -1, COLOR_WHITE);
		init_pair(9 + COLOR_RED,     -1, COLOR_RED);
		init_pair(9 + COLOR_CYAN,    -1, COLOR_CYAN);
		init_pair(9 + COLOR_MAGENTA, -1, COLOR_MAGENTA);
		init_pair(9 + COLOR_BLUE,    -1, COLOR_BLUE);
		init_pair(9 + COLOR_YELLOW,  -1, COLOR_YELLOW);
	}
}

void nc_term()
{
	endwin();
}

void nc_cls()
{
	erase();
}

void nc_vstatus(const char *fmt, va_list l)
{
	move(LINES - 1, 0);
	clrtoeol();
	vwprintw(stdscr, fmt, l);
}

void nc_get_yx(int *y, int *x)
{
	getyx(stdscr, *y, *x);
}

void nc_set_yx(int y, int x)
{
	move(y, x);
}

int nc_getch()
{
	/* TODO: interrupts, winch */
	return getch();
}

void nc_addch(char c)
{
	addch(c);
}

void nc_addstr(char *s)
{
	addstr(s);
}

int nc_LINES(void)
{
	return LINES;
}

void nc_clrtoeol()
{
	clrtoeol();
}
