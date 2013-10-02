#ifndef IO_H
#define IO_H

#include "map.h"

enum io
{
	IO_MAP,
	IO_NOMAP
};

int io_getch(enum io);

#endif
