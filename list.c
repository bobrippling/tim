#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "mem.h"

list_t *list_new()
{
	list_t *l = umalloc(sizeof *l);
	return l;
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

void list_inschar(list_t *l, int x, int y, char ch)
{
	int i;

	l = list_seek(l, y);

	if(x >= l->len_malloc){
		const int old_len = l->len_malloc;

		l->len_malloc = x + 1;
		l->line = urealloc(l->line, l->len_malloc);

		memset(l->line + old_len, 0, l->len_malloc - old_len);

		for(i = l->len_line; i < x; i++){
			l->line[i] = ' ';
			l->len_line++;
		}
	}else{
		/* shift stuff up */
		l->line = urealloc(l->line, ++l->len_malloc);
		memmove(l->line + x + 1, l->line + x, l->len_line - x);
	}

	l->line[x] = ch;

	l->len_line++;
}

void list_delchar(list_t *l, int x, int y)
{
	l = list_seek(l, y);

	if(x >= l->len_line)
		return;

	memmove(l->line + x, l->line + x + 1, l->len_line - x);

	l->len_line--;
}