#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "buffer.h"
#include "mem.h"

buffer_t *buffer_new()
{
	buffer_t *b = umalloc(sizeof *b);
	b->head = list_new();
	return b;
}

void buffer_inschar(buffer_t *buf, int x, int y, char ch)
{
	list_inschar(buf->head, x, y, ch);
}

void buffer_delchar(buffer_t *buf, int x, int y)
{
	list_delchar(buf->head, x, y);
}
