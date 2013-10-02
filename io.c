#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ncurses.h"
#include "mem.h"
#include "io.h"

extern const keymap_t maps[];

static char *io_fifo;
static size_t io_fifoused, io_fifosz;

static void io_fifo_push(const char *s)
{
	size_t newlen = strlen(s);
	size_t start = io_fifoused;

	if(io_fifoused + newlen > io_fifosz){
		io_fifo = urealloc(io_fifo,
				sizeof(*io_fifo) * (io_fifosz = io_fifoused + newlen));
	}

	memcpy(io_fifo + start, s, newlen);
	io_fifoused += newlen;
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

static void io_map(int ch)
{
	for(const keymap_t *m = maps; m->to; m++)
		if(m->from == ch)
			io_fifo_push(m->to);
}

int io_getch(enum io ty)
{
	if(io_fifoused)
		return io_fifo_pop();

	int ch = nc_getch();

	switch(ty){
		case IO_MAP:
			io_map(ch);
			if(io_fifoused)
				return io_fifo_pop();
			/* fall */

		case IO_NOMAP:
			break;
	}

	return ch;
}
