#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <stdio.h>

#include "ncurses.h"
#include "mem.h"
#include "io.h"
#include "bufmode.h"
#include "map.h"
#include "str.h"

#ifndef CHAR_BIT
#  define CHAR_BIT 8
#endif

static struct fifo_ent
{
	unsigned ch : CHAR_BIT * sizeof(char);
	unsigned needs_map : 1;
} *io_fifo;

static size_t io_fifoused, io_fifosz;

size_t io_pending(char buf[], size_t len)
{
	if(buf)
		for(size_t i = 0; i < len; i++)
			buf[i] = io_fifo[i].ch;

	return io_fifoused;
}

static void io_fifo_realloc(size_t len)
{
	if(io_fifoused + len > io_fifosz){
		io_fifo = urealloc(io_fifo,
				sizeof(*io_fifo) * (io_fifosz = io_fifoused + len));
	}
	io_fifoused += len;
}

static void io_fifo_ins(char c, bool need_map)
{
	io_fifo_realloc(1);
	memmove(io_fifo + 1, io_fifo, (io_fifoused - 1) * sizeof(*io_fifo));
	io_fifo->ch = c;
	io_fifo->needs_map = need_map;
}

static int io_fifo_pop(bool *need_map)
{
	/* pull a char from the fifo */
	int ret = io_fifo->ch;
	*need_map = io_fifo->needs_map;

	io_fifoused--;
	if(io_fifoused)
		memmove(io_fifo, io_fifo + 1, sizeof(*io_fifo) * io_fifoused);

	return ret;
}

static const keymap_t *io_findmap(int ch, enum io mode_mask)
{
	extern const keymap_t maps[];

	for(const keymap_t *m = maps; m->to; m++)
		if(m->mode & mode_mask && m->from == ch)
			return m;

	return NULL;
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

int io_getch(enum io ty, bool *wasraw, bool domaps)
{
	if(wasraw)
		*wasraw = false;

	int ch;
	bool need_map = false;
	if(io_fifoused)
		ch = io_fifo_pop(&need_map);
	else
		ch = nc_getch(ty & IO_MAPRAW, wasraw);

	/* don't run maps for raw keys */
	if(domaps && (!wasraw || !*wasraw || need_map)){
		const keymap_t *map = io_findmap(ch, ty & ~IO_MAPRAW);

		if(map){
			for(size_t i = strlen(map->to); i > 0; i--)
				io_ungetch(map->to[i-1], false);

			ch = io_fifo_pop(&need_map);
			assert(!need_map); /* corresponds to the false above */
			return ch;
		}
	}

	return ch;
}

void io_ungetch(int ch, bool needmap)
{
	io_fifo_ins(ch, needmap);
}

unsigned io_read_repeat(enum io io_mode)
{
	unsigned repeat = 0;

	bool raw;
	int ch = io_getch(io_mode, &raw, true);
	(void)raw;

	if(isdigit(ch) && ch != '0'){
		repeat = ch - '0';
		/* more repeats */
		for(;;){
			ch = io_getch(io_mode, &raw, true);
			if('0' <= ch && ch <= '9')
				repeat = repeat * 10 + ch - '0';
			else
				break;
		}
	}

	io_ungetch(ch, false);
	return repeat;
}
