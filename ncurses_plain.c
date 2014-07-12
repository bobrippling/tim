#include <stdio.h>
#include <unistd.h>

#include "ncurses.h"
#include "macros.h"

#define LINES 25
#define COLS 80

static int nc_x, nc_y;

void nc_init()
{
}

void nc_term()
{
}

void nc_clearall()
{
	printf("clear\n");
}

void nc_highlight(int on)
{
	(void)on;
}

void nc_vstatus(const char *fmt, va_list l, int right)
{
	printf("status %s: ", right ? "right" : "left");

	vprintf(fmt, l);

	putchar('\n');
}

void nc_get_yx(int *y, int *x)
{
	*x = nc_x;
	*y = nc_y;
}

void nc_set_yx(int y, int x)
{
	nc_x = x;
	nc_y = y;
}

int nc_getch(bool mapraw, bool *wasraw)
{
	char ch;
	int r = read(0, &ch, 1);
	if(r <= 0)
		return -1;
	return ch;
}

void nc_addch(char c)
{
	printf("ch: '%c'\n", c);
}

void nc_style(enum nc_style s)
{
	(void)s;
}

void nc_addstr(char *s)
{
	printf("str: \"%s\"", s);
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
	printf("clrtoeol()\n");
}
