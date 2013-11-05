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

void yank_put_in_list(
		const yank *ynk, list_t **phead,
		const bool prepend,
		int *py, int *px)
{
	switch(ynk->as){
		case REGION_CHAR:
		{
			extern void ui_status();
			ui_status("can't do char yank-put yet");
			break;
		}
		case REGION_LINE:
		{
			list_t *copy = list_copy_deep(ynk->list, NULL);
			list_t *copy_tail = list_tail(copy);
			list_t *const head = *phead;

			if(prepend){
				*phead = copy;
				copy_tail->next = head;
			}else{
				list_t *const after = head->next;

				/* top link */
				head->next = copy;
				copy->prev = head;

				/* bottom link */
				copy_tail->next = after;
				if(after)
					after->prev = copy_tail;

				++*py;
			}
			break;
		}
		case REGION_COL:
		{
			list_t *head = *phead;
			for(const list_t *l = ynk->list;
			    l;
			    l = l->next, head = list_seek(head, 1, l))
			{
				int x = *px + !prepend;
				int y = 0;
				for(unsigned i = 0; i < l->len_line; i++)
					list_inschar(head, &x, &y, l->line[i]);
			}

			++*px;
			break;
		}
	}
}
