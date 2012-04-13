#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "mem.h"

list_t *list_new()
{
	list_t *l = umalloc(sizeof *l);
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
	l = list_new();

	while((ch = fgetc(f)) != EOF)
		list_inschar(l, &x, &y, ch);

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

list_t *list_seek(list_t *l, int y)
{
	while(y > 0){
		if(!l->next)
			l->next = list_new();

		l = l->next;
		y--;
	}

	return l;
}

void list_inschar(list_t *l, int *x, int *y, char ch)
{
	int i;

	l = list_seek(l, *y);

	if(*x >= l->len_malloc){
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
		memmove(l->line + *x + 1, l->line + *x, l->len_line - *x);
	}

	if(ch == '\n' || ch == '\r'){
		char *cut;
		int cut_len;
		list_t *next;

		if(l->line){
			cut = ustrdup(l->line + *x);
			cut_len = l->len_line - *x;
			l->len_line = *x;
		}else{
			cut = NULL;
			cut_len = 0;
		}

		next = l->next;
		l->next = list_new();
		l->next->next = next;

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
	l = list_seek(l, *y);

	if(*x > l->len_line)
		return;

	--*x;

	memmove(l->line + *x, l->line + *x + 1, l->len_line - *x);

	l->len_line--;
}

int list_count(list_t *l)
{
	int i;
	for(i = 0; l->next; l = l->next, i++);
	return i;
}
