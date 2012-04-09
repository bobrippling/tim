#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "buffer.h"
#include "mem.h"

void buffer_free(buffer_t *b)
{
	list_free(b->head);
	free(b);
}

buffer_t *buffer_new()
{
	buffer_t *b = umalloc(sizeof *b);
	b->head = list_new();
	return b;
}

buffer_t *buffer_new_file(FILE *f)
{
	/* TODO: mmap() */
	list_t *l;
	buffer_t *b;

	l = list_new_file(f);

	if(!l)
		return NULL;

	b = buffer_new();
	b->head = l;

	return b;
}

buffer_t *buffer_new_fname(const char *fname)
{
	buffer_t *b;
	FILE *f;

	f = fopen(fname, "r");
	if(!f)
		return NULL;

	b = buffer_new_file(f);
	fclose(f);

	return b;
}

void buffer_inschar(buffer_t *buf, int *x, int *y, char ch)
{
	list_inschar(buf->head, x, y, ch);
}

void buffer_delchar(buffer_t *buf, int *x, int *y)
{
	list_delchar(buf->head, x, y);
}
