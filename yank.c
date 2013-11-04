#include <stdio.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "yank.h"
#include "mem.h"

struct yank
{
	list_t *list;
	enum region_type as;
};

static yank *yank_buf;

yank *yank_new(list_t *l, enum region_type as)
{
	yank *y = umalloc(sizeof *y);
	y->list = l;
	y->as = as;
	return y;
}

static void yank_free(yank *y)
{
	list_free(y->list);
	free(y);
}

void yank_push(yank *y)
{
	if(yank_buf)
		yank_free(yank_buf);
	yank_buf = y;
}

const yank *yank_top()
{
	return yank_buf;
}

void yank_put_in_list(const yank *y, list_t *head)
{
	list_t *copy = list_copy_deep(y->list, NULL);
	list_t *copy_tail = list_tail(copy);

	list_t *const after = head->next;

	/* top link */
	head->next = copy;
	copy->prev = head;

	/* bottom link */
	copy_tail->next = after;
	after->prev = copy_tail;
}
