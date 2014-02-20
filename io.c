#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ncurses.h"
#include "mem.h"
#include "io.h"
#include "bufmode.h"
#include "map.h"

extern const keymap_t maps[];

static char *io_fifo;
static size_t io_fifoused, io_fifosz;

static void io_fifo_realloc(size_t len)
{
	if(io_fifoused + len > io_fifosz){
		io_fifo = urealloc(io_fifo,
				sizeof(*io_fifo) * (io_fifosz = io_fifoused + len));
	}
	io_fifoused += len;
}

static void io_fifo_push(const char *s)
{
	if(!s)
		return;

	size_t newlen = strlen(s);
	size_t start = io_fifoused;

	io_fifo_realloc(newlen);

	memcpy(io_fifo + start, s, newlen);
}

static void io_fifo_ins(char c)
{
	io_fifo_realloc(1);
	memmove(io_fifo + 1, io_fifo, io_fifoused - 1);
	*io_fifo = c;
}

static int io_fifo_pop(void)
{
	/* pull a char from the fifo */
	int ret = *io_fifo;

	io_fifoused--;
	if(io_fifoused)
		memmove(io_fifo, io_fifo + 1, sizeof(*io_fifo) * io_fifoused);

	return ret;
}

static void io_map(int ch, enum io mode_mask)
{
	for(const keymap_t *m = maps; m->to; m++)
		if(m->mode & mode_mask && m->from == ch)
			io_fifo_push(m->to);
}

size_t io_bufsz(void)
{
	return io_fifoused;
}

enum io bufmode_to_iomap(enum buf_mode bufmode)
{
	if(bufmode & UI_INSERT_ANY)
		return IO_MAPI;

	if(bufmode & UI_VISUAL_ANY)
		return IO_MAPV;

	return IO_MAP;
}

int io_getch(enum io ty, bool *wasraw)
{
	if(wasraw)
		*wasraw = false;

	if(io_fifoused)
		return io_fifo_pop();

	int ch = nc_getch(ty & IO_MAPRAW, wasraw);

	/* don't run maps for raw keys */
	if(!wasraw || !*wasraw){
		io_map(ch, ty & ~IO_MAPRAW);
		if(io_fifoused)
			return io_fifo_pop();
	}

	return ch;
}

void io_ungetch(int ch)
{
	io_fifo_ins(ch);
}

void io_ungetstrr(const char *s, size_t n)
{
	if(!*s)
		return;

	for(size_t i = n - 1;
			i < n;
			i--)
	{
		io_ungetch(s[i]);
	}
}

unsigned io_read_repeat(enum io io_mode)
{
	unsigned repeat = 1;

	int ch = io_getch(io_mode, NULL);
	if(isdigit(ch) && ch != '0'){
		repeat = ch - '0';
		/* more repeats */
		for(;;){
			ch = io_getch(io_mode, NULL);
			if('0' <= ch && ch <= '9')
				repeat = repeat * 10 + ch - '0';
			else
				break;
		}
	}

	io_ungetch(ch);
	return repeat;
}
