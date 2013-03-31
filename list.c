#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pos.h"
#include "list.h"
#include "mem.h"

list_t *list_new(list_t *prev)
{
	list_t *l = umalloc(sizeof *l);
	l->prev = prev;
	return l;
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
		list_free(l->next);
		free(l->line);
		free(l);
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

	if(ch == '\n' || ch == '\r'){
		char *cut;
		int cut_len;

		if(l->line){
			cut_len = l->len_line - *x;

			if(cut_len > 0){
				cut = umalloc(cut_len);
				memcpy(cut, l->line + *x + 1, cut_len);
			}else{
				cut = NULL;
				cut_len = 0;
			}

			l->len_line = *x;
		}else{
			cut = NULL;
			cut_len = 0;
		}

		/* insert a line after */
		list_insline(&l, &(int){0}, &(int){0}, 1);

		l = l->next;
		l->line = cut;
		l->len_malloc = l->len_line = cut_len;

		*x = 0;
		++*y;

	}else{
		l->line[*x] = ch;
		l->len_line++;
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

void list_delbetween(list_t **pl,
		point_t const *from, point_t const *to,
		int linewise)
{
	assert(from->y <= to->y);
	assert(from->y < to->y || from->x < to->x);

	list_t **seeked = list_seekp(pl, from->y, 0);

	if(!seeked || !*seeked)
		return;

	if(linewise){
		list_dellines(seeked, (*seeked)->prev, to->y - from->y + 1);
	}else{
		list_t *l = *seeked;
		size_t line_change = to->y - from->y;

		if(line_change > 1)
			list_dellines(&l->next, l, line_change);

		if(line_change > 0){
			/* join the lines */
			list_t *next = l->next;
			size_t nextlen = next->len_line - to->x;
			size_t fulllen = from->x + nextlen;

			if(l->len_malloc < fulllen)
				l->line = urealloc(l->line, l->len_malloc = fulllen);

			memcpy(l->line + from->x,
					next->line + to->x,
					nextlen);
			l->len_line = fulllen;

			list_dellines(&l->next, l, 1);

		}else{
			if(!l->len_line || (unsigned)to->x > l->len_line)
				return;

			size_t diff = to->x - from->x;

			memmove(
					l->line + from->x,
					l->line + to->x,
					l->len_line - from->x - 1);

			l->len_line -= diff;
		}
	}
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

void list_replace_at(list_t *l, int *px, int y, char *with)
{
	l = list_seek(l, y, 1);

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

	*px += with_len - 1;
}

int list_count(list_t *l)
{
	int i;
	for(i = 0; l->next; l = l->next, i++);
	return i;
}
