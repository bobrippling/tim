#include <stdio.h>
#include <stdlib.h>

#include "pos.h"
#include "list.h"
#include "yank.h"

static list_t *yank_buf;

void yank_push(list_t *l)
{
	/* XXX: memleak */
	yank_buf = l;
}

list_t *yank_top()
{
	return yank_buf;
}
