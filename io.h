#ifndef IO_H
#define IO_H

#include "map.h"

enum io
{
	/* one of: */
	IO_NOMAP = 0,
	IO_MAP   = 1,
	IO_MAPV  = 2, /* use the visual entry? */

	/* optional, or'd in */
	IO_MAPRAW = 4, /* change ^Vx to literal x */
};

int io_getch(enum io);
void io_ungetch(int);
unsigned io_read_repeat(enum io);

#define K_ESC '\033'

#endif
