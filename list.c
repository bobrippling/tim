#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "mem.h"
#include "str.h"

list_t *list_new(list_t *prev)
{
	list_t *l = umalloc(sizeof *l);
	l->prev = prev;
	return l;
}

list_t *list_new_fd(int fd)
{
	list_t *head = NULL, **p_next = &head;

	/* TODO: mmap() */
}

list_t *list_new_file(FILE *f)
{
#ifdef FAST_LIST_INIT
	char buf[256];
	size_t n;
	list_t *l, *head;

	l = head = list_new();

	while((n = fread(buf, sizeof *buf, sizeof buf, f)) > 0){
		char *nl = memchr(buf, '\n', n);
		if(!nl)
			abort(); /* TODO: */

		l->len_malloc = l->len_line = nl - buf;

		if(l->len_malloc){
			l->line = umalloc(len);

		}
	}

	if(n < 0){
		list_free(l);
		return NULL;
	}

	return l;
#else
	int ch;
	int y, x;
	list_t *l;

	y = x = 0;
	l = list_new(NULL);

	while((ch = fgetc(f)) != EOF)
		list_inschar(&l, &x, &y, ch);

	return l;
#endif
}

void list_free(list_t *l)
{
	if(l){
		list_t *next = l->next;

		free(l->line);
		free(l);

		/* tco */
		list_free(next);
	}
}

static
list_t **list_seekp(list_t **pl, int y, int creat)
{
	if(!*pl){
		if(creat)
			*pl = list_new(NULL);
		else
			return NULL;
	}

	while(y > 0){
		if(!(*pl)->next){
			if(creat)
				(*pl)->next = list_new(*pl);
			else
				return NULL;
		}

		pl = &(*pl)->next;
		y--;
	}

	return pl;
}

list_t *list_seek(list_t *l, int y, int creat)
{
	list_t **p = list_seekp(&l, y, creat);
	return p ? *p : NULL;
}

static int list_evalnewlines(list_t *l)
{
	if(l->len_line == 0)
		return 0;

	int r = 0;
	for(size_t i = l->len_line - 1; ; i--){
		char ch = l->line[i];

		if(isnewline(ch)){
			int cut_len = l->len_line - i;
			char *cut;
			if(cut_len > 0){
				cut = umalloc(cut_len);
				memcpy(cut, l->line + i + 1, cut_len);
			}else{
				cut = NULL;
				cut_len = 0;
			}

			l->len_line = i;

			/* insert a line after */
			list_insline(&l, &(int){0}, &(int){0}, 1);

			list_t *nl = l->next;
			nl->line = cut;
			nl->len_malloc = cut_len;
			nl->len_line = cut_len - 1;

			r = 1; /* we replaced */
		}

		if(i == 0)
			break;
	}

	return r;
}

void list_inschar(list_t **pl, int *x, int *y, char ch)
{
	list_t *l;
	int i;

	l = *(pl = list_seekp(pl, *y, 1));

	if((unsigned)*x >= l->len_malloc){
		const int old_len = l->len_malloc;

		l->len_malloc = *x + 1;
		l->line = urealloc(l->line, l->len_malloc);

		memset(l->line + old_len, 0, l->len_malloc - old_len);

		for(i = l->len_line; i < *x; i++){
			l->line[i] = ' ';
			l->len_line++;
		}
	}else{
		/* shift stuff up */
		l->line = urealloc(l->line, ++l->len_malloc);
		memmove(l->line + *x + 1, l->line + *x, l->len_malloc - *x - 1);
	}

	l->line[*x] = ch;
	l->len_line++;
	if(list_evalnewlines(l)){
		*x = 0;
		++*y;
	}else{
		++*x;
	}
}

void list_delchar(list_t *l, int *x, int *y)
{
	l = list_seek(l, *y, 0);

	--*x;

	if(!l)
		return;

	if((unsigned)*x >= l->len_line)
		return;

	memmove(l->line + *x, l->line + *x + 1, --l->len_line - *x);
}

static
void list_dellines(list_t **pl, list_t *prev, unsigned n)
{
	if(n == 0)
		return;

	list_t *l = *pl;

	if(!l)
		return;

	if(n == 1){
		list_t *adv = l->next;
		l->next = NULL;

		*pl = adv;

	}else{
		list_t *end_m1 = list_seek(l, n - 2, 0);

		if(end_m1){
			*pl = end_m1->next;
			end_m1->next = NULL;
		}else{
			/* delete everything */
			*pl = NULL;
		}
	}

	if(*pl)
		(*pl)->prev = prev;

	list_free(l);
}

void list_delregion(list_t **pl, const region_t *region)
{
	assert(region->begin.y <= region->end.y);
	assert(region->begin.y < region->end.y || region->begin.x < region->end.x);

	list_t **seeked = list_seekp(pl, region->begin.y, 0);

	if(!seeked || !*seeked)
		return;

	switch(region->type){
		case REGION_LINE:
			list_dellines(seeked, (*seeked)->prev, region->end.y - region->begin.y + 1);
			break;
		case REGION_CHAR:
		{
			list_t *l = *seeked;
			size_t line_change = region->end.y - region->begin.y;

			if(line_change > 1)
				list_dellines(&l->next, l, line_change);

			if(line_change > 0){
				/* join the lines */
				list_t *next = l->next;
				size_t nextlen = next->len_line - region->end.x;
				size_t fulllen = region->begin.x + nextlen;

				if(l->len_malloc < fulllen)
					l->line = urealloc(l->line, l->len_malloc = fulllen);

				memcpy(l->line + region->begin.x,
						next->line + region->end.x,
						nextlen);
				l->len_line = fulllen;

				list_dellines(&l->next, l, 1);

			}else{
				if(!l->len_line || (unsigned)region->end.x > l->len_line)
					return;

				size_t diff = region->end.x - region->begin.x;

				memmove(
						l->line + region->begin.x,
						l->line + region->end.x,
						l->len_line - region->begin.x - 1);

				l->len_line -= diff;
			}
			break;
		}
		case REGION_COL:
		{
			list_t *pos = *seeked;
			point_t begin = region->begin, end = region->end;

			/* should've been sorted also */
			assert(begin.x <= end.x);

			for(int i = begin.y;
					i <= end.y && pos;
					i++, pos = pos->next)
			{
				if((unsigned)begin.x < pos->len_line){
					if((unsigned)end.x >= pos->len_line){
						/* delete all */
						pos->len_line = begin.x;

					}else{
						char *str = pos->line;

						memmove(
								str + begin.x,
								str + end.x,
								pos->len_line - end.x);

						pos->len_line -= end.x - begin.x;
					}
				}
			}
			break;
		}
	}
}

void list_joinregion(list_t **pl, const region_t *region)
{
	if(region->begin.y == region->end.y)
		return;

	assert(region->begin.y < region->end.y);

	list_t *l, *start = list_seek(*pl, region->begin.y, 0);
	int i;

	if(!start)
		return;

	if(start->line)
		str_rtrim(start->line, &start->len_line);

	for(l = start->next, i = region->begin.y + 1;
			l && i < region->end.y;
			l = l->next, i++)
	{
		if(!l->line)
			continue;

		str_ltrim(l->line, &l->len_line);

		/* + 1 for \0, +1 for ' ' */
		const size_t target = start->len_line + l->len_line + 2;

		if(start->len_malloc < target)
			start->len_malloc = target;

		start->line = urealloc(
				start->line,
				start->len_malloc);

		start->line[start->len_line++] = ' ';

		memcpy(start->line + start->len_line,
				l->line,
				l->len_line);

		start->len_line += l->len_line;
	}

	if(i > region->begin.y)
		list_dellines(&start->next, start, i - region->begin.y);
}

void list_insline(list_t **pl, int *x, int *y, int dir)
{
	list_t *l, *save;

	*x = 0;

	if(dir < 0 && *y == 0){
		/* special case */
		l = *pl;
		(*pl = list_new(NULL))->next = l;
		if(l)
			l->prev = *pl;
		return;
	}

	if(dir < 0)
		--*y;

	l = list_seek(*pl, *y, 1);

	save = l->next;
	l->next = list_new(l);
	l->next->next = save;
	if(save)
		save->prev = l->next;

	++*y;
}

void list_replace_at(list_t *l, int *px, int *py, char *with)
{
	l = list_seek(l, *py, 1);

	const int with_len = strlen(with);
	int x = *px;

	if((unsigned)(x + with_len) >= l->len_malloc){
		size_t new_len = x + with_len + 1;
		l->line = urealloc(l->line, new_len);
		memset(l->line + l->len_malloc, ' ', new_len - l->len_malloc);
		l->len_line = l->len_malloc = new_len;
	}

	char *p = l->line + x;
	memcpy(p, with, with_len);

	/* convert r\n to newlines */
	if(list_evalnewlines(l)){
		*px = 0;
		++*py;
	}else{
		*px += with_len - 1;
	}
}

int list_count(list_t *l)
{
	int i;
	for(i = 0; l->next; l = l->next, i++);
	return i;
}

int list_filter(list_t **pl,
                point_t const *from,
                point_t const *to,
                const char *cmd)
{
	pl = list_seekp(pl, from->y, 0);

	if(!pl)
		return;

	int child_in[2], child_out[2];
	if(pipe(child_in))
		return -1;
	if(pipe(child_out))
		goto close_pipe_in;

	switch(fork()){
		case -1:
			goto close_pipe_both;
		case 0:
			dup2(child_in[0], 0);
			dup2(child_out[1], 1);
			for(int i = 0; i < 2; i++)
				close(child_in[i]), close(child_out[i]);

			execl("/bin/sh", "sh", "-c", cmd);
			exit(127);
	}

	/* parent */
	close(child_in[0]), close(child_out[1]);

	/* write our lines to child_in */
	size_t i = from.y;
	for(list_t *l = *pl; l && i <= to.y; l = l->next, i++){
		char *s;
		size_t len;

		if(l->len_line)
			s = l->line, len = l->len_line;
		else
			s = "\n", len = 1;

		int r = write(child_in[1], s, len);
		if(r == -1)
			break;
	}

	/* read from child_out */
	list_t *l_read = list_new_fd(child_out[0]);

close_pipe_both:
	{
		const int e = errno;
		close(child_out[0]);
		close(child_out[1]);
		errno = e;
	}
close_pipe_in:
	{
		const int e = errno;
		close(child_in[0]);
		close(child_in[1]);
		errno = e;
	}
	return -1;
}
