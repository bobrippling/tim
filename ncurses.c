#include <ncurses.h>
#include <signal.h>
#include <ctype.h>

#include "ncurses.h"
#include "macros.h"

void nc_init()
{
	static int init = 0;

	if(!init){
		init = 1;

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
	}else{
		refresh();
	}
}

void nc_term()
{
	endwin();
}

void nc_clearall()
{
	clear();
}

void nc_highlight(int on)
{
	(on ? attron : attroff)(A_REVERSE);
}

void nc_vstatus(const char *fmt, va_list l, int right)
{
	scrollok(stdscr, 0);

	int x = 0;
	if(right)
		x = COLS / 2;

	move(LINES - 1, x);
	clrtoeol();
	vwprintw(stdscr, fmt, l);

	scrollok(stdscr, 1);
}

void nc_get_yx(int *y, int *x)
{
	getyx(stdscr, *y, *x);
}

void nc_set_yx(int y, int x)
{
	move(y, x);
}

int nc_charlen(int ch)
{
	int l = 1;
	switch(ch){
		case '\r':
			l++; /* ^M */
			break;
		case '\t':
			break;
		default:
			if(!isprint(ch))
				l++; /* ^X */
	}
	return l;
}

int nc_getch(bool mapraw, bool *wasraw)
{
	/* TODO: interrupts, winch */
	bool ctrl_v = false;
	int ch;
restart:
	ch = getch();

	if(ctrl_v)
		return ch;

	if(ch == CTRL_AND('z')){
		raise(SIGTSTP);
		goto restart;
	}
	if(mapraw){
		if(ch == '\r'){
			ch = '\n';
		}else if(ch == CTRL_AND('v')){
			if(wasraw)
				*wasraw = true;
			ctrl_v = true;
			goto restart;
		}
		if(wasraw)
			*wasraw = false;
	}
	return ch;
}

void nc_addch(char c)
{
	if(c == '\t'){
		c = ' ';
	}else if(c == '\r'){
		/* TODO: all non-printables */
		addch('^');
		addch('M');
		return;
	}

	addch(c & 0xff);
}

void nc_style(enum nc_style s)
{
	int to_set = A_NORMAL;
	int col = -1;

	if(s & COL_BLUE)
		col = COLOR_BLUE;
	else if(s & COL_BROWN)
		col = COLOR_YELLOW;

	if(col != -1)
		to_set |= COLOR_PAIR(1 + col);

	if(s & COL_BG_RED)
		to_set |= COLOR_PAIR(9 + COLOR_RED);

	if(s & ATTR_BOLD)
		to_set |= A_BOLD;

	attrset(to_set);
}

void nc_addstr(char *s)
{
	addstr(s);
}

int nc_LINES(void)
{
	return LINES;
}

int nc_COLS(void)
{
	return COLS;
}

void nc_clrtoeol()
{
	clrtoeol();
}
