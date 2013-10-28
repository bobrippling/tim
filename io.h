#ifndef IO_H
#define IO_H

#include "map.h"

enum io
{
	IO_NOMAP,
	IO_MAP,
	IO_MAPV, /* use the visual entry? */
};

int io_getch(enum io);
void io_ungetch(int);
unsigned io_read_repeat(enum io);

#define K_ESC '\033'

#endif
