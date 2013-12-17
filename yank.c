#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void charwise_put(
		const yank *ynk, list_t *head,
		int *px, int *py, bool prepend)
{
	/* first line */
	const list_t *ins_iter = ynk->list;

	*px += !prepend;

	for(unsigned i = 0; i < ins_iter->len_line; i++)
		list_inschar(head,
				px,
				&(int){ 0 },
				ins_iter->line[i]);

	/* if we have multiple lines... */
	if((ins_iter = ins_iter->next)){
		struct str
		{
			char *str;
			size_t len;
		};

		/* need to pull out the rest of the line */
		struct str append = { .str = NULL };

		unsigned off = *px;
		if(off < head->len_line){
			unsigned len = head->len_line - *px;

			append.str = ustrdup_len(head->line + *px, len);
			append.len = len;

			head->len_line = *px;
		}

		/* append ins_iter - 2 lines */
		list_append(head, head, list_copy_deep(ins_iter, /*prev:*/head));

		if(append.str){
			for(; ins_iter; ins_iter = ins_iter->next)
				head = head->next;

			/* head is now the last inserted line */
			struct str save = { head->line, head->len_line };
			head->line = append.str;
			head->len_line = append.len;

			unsigned len = head->len_line + append.len;
			head->len_malloc = len + 1;
			head->line = urealloc(head->line, head->len_malloc);

			memcpy(head->line + head->len_line,
					save.str,
					save.len);

			head->len_line += save.len;
		}

		if(!prepend)
			++*px;
	}else{
		*px += ynk->list->len_line - prepend;
	}
}

void yank_put_in_list(
		const yank *ynk, list_t **phead,
		const bool prepend,
		int *py, int *px)
{
	switch(ynk->as){
		case REGION_CHAR:
			charwise_put(ynk, *phead, px, py, prepend);
			break;

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
			int y = 0;
			for(const list_t *l = ynk->list;
			    l; l = l->next, y++)
			{
				for(unsigned i = 0; i < l->len_line; i++)
					list_inschar(head,
							&(int){ *px + !prepend + i },
							&(int){ y },
							l->line[i]);
			}

			if(!prepend)
				++*px;
			break;
		}
	}
}
