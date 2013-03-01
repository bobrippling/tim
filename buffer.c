#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "list.h"
#include "pos.h"
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

static
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

void buffer_new_fname(buffer_t **pb, const char *fname, int *err)
{
	buffer_t *b;
	FILE *f;

	f = fopen(fname, "r");
	if(!f){
got_err:
		*err = 1;
		b = buffer_new();
		goto fin;
	}

	b = buffer_new_file(f);
	fclose(f);

	if(!b)
		goto got_err;

	*err = 0;

fin:
	buffer_set_fname(b, fname);
	*pb = b;
}

int buffer_replace_file(buffer_t *b, FILE *f)
{
	list_t *l = list_new_file(f);

	if(!l)
		return 0;

	list_free(b->head);
	b->head = l;

	return 1;
}

int buffer_replace_fname(buffer_t *b, const char *fname)
{
	FILE *f = fopen(fname, "r");
	int r;

	if(!f)
		return 0;

	r = buffer_replace_file(b, f);
	fclose(f);

	return r;
}

void buffer_set_fname(buffer_t *b, const char *s)
{
	if(b->fname != s){
		free(b->fname);
		b->fname = ustrdup(s);
	}
}

const char *buffer_fname(buffer_t *b)
{
	return b->fname;
}

void buffer_inschar(buffer_t *buf, int *x, int *y, char ch)
{
	list_inschar(buf->head, x, y, ch);
}

void buffer_delchar(buffer_t *buf, int *x, int *y)
{
	list_delchar(buf->head, x, y);
}

void buffer_insline(buffer_t *buf, int dir)
{
	list_insline(&buf->head, &buf->ui_pos.x, &buf->ui_pos.y, dir);
}

buffer_t *buffer_topleftmost(buffer_t *b)
{
	for(;;){
		int changed = 0;
		for(; b->neighbours[BUF_LEFT]; changed = 1, b = b->neighbours[BUF_LEFT]);
		for(; b->neighbours[BUF_UP];   changed = 1, b = b->neighbours[BUF_UP]);
		if(!changed)
			break;
	}

	return b;
}

void buffer_add_neighbour(buffer_t *to, enum buffer_neighbour loc, buffer_t *new)
{
	/* TODO; leaks, etc */
	//buffer_t *sav = to->neighbours[loc];
	enum buffer_neighbour rloc;

#define OPPOSITE(a, b) case a: rloc = b; break

	switch(loc){
		OPPOSITE(BUF_LEFT,  BUF_RIGHT);
		OPPOSITE(BUF_RIGHT, BUF_LEFT);
		OPPOSITE(BUF_DOWN,  BUF_UP);
		OPPOSITE(BUF_UP,    BUF_DOWN);
	}

	to->neighbours[loc] = new;
	new->neighbours[rloc] = to;
}

list_t *buffer_current_line(buffer_t *b)
{
	return list_seek(b->head, b->ui_pos.y, 0);
}
