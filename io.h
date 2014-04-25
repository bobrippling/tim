#ifndef IO_H
#define IO_H

#include "bufmode.h"

enum io
{
	IO_NOMAP = 1 << 0,
	IO_MAP   = 1 << 1,
	IO_MAPV  = 1 << 2,
	IO_MAPI  = 1 << 3,

	IO_MAPRAW = 1 << 4, /* change ^Vx to literal x */
};

enum io bufmode_to_iomap(enum buf_mode);

int io_getch(enum io, bool *wasraw);
unsigned io_read_repeat(enum io);

void io_ungetch(int ch, bool allow_map_on_out);

size_t io_bufsz(void);

#define K_ESC '\033'

#endif
