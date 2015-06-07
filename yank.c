#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "yank.h"
#include "mem.h"
#include "retain.h"

struct yank
{
	struct retain;
	list_t *list;
	enum region_type as;
};

static yank *yank_buf;

yank *yank_new(list_t *l, enum region_type as)
{
	yank *y = retain(umalloc(sizeof *y));
	assert(l && !l->prev);
	y->list = l;
	y->as = as;
	return y;
}

void yank_free(const yank *y)
{
	list_free(y->list);
	free((yank *)y);
}

yank *yank_push(yank *y)
{
	if(yank_buf)
		release(yank_buf, yank_free);
	yank_buf = retain(y);
	return yank_buf;
}

const yank *yank_top()
{
	return yank_buf;
}

static void charwise_put(
		const yank *ynk, list_t *head,
		int *px, bool prepend)
{
	/* first line */
	const list_t *ins_iter = ynk->list;

	*px += !prepend;

	for(unsigned i = 0; i < ins_iter->len_line; i++)
		list_inschar(head,
				px,
				&(int){ 0 },
				ins_iter->line[i], /*autogap*/0);

	int final_x = *px - 1;

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
		{
			list_t *copy = list_copy_deep(ins_iter, NULL);

			list_t *copy_tail = list_tail(copy);
			list_t *after = head->next;
			head->next = copy;
			copy->prev = head;

			copy_tail->next = after;
			if(after)
				after->prev = copy_tail;
		}

		if(append.str){
			list_t *tail = head;
			for(; ins_iter; ins_iter = ins_iter->next)
				tail = tail->next;

			/* tail is now the last inserted line */
			unsigned len = tail->len_line + append.len;
			tail->len_malloc = len + 1;
			tail->line = urealloc(tail->line, tail->len_malloc);

			memcpy(tail->line + tail->len_line,
					append.str,
					append.len);

			tail->len_line += append.len;
		}
	}

	*px = final_x;
}

void yank_put_in_list(
		const yank *ynk,
		list_t **const phead,
		const bool prepend,
		int *py, int *px)
{
	switch(ynk->as){
		case REGION_CHAR:
			charwise_put(ynk, list_seek(*phead, *py, true), px, prepend);
			break;

		case REGION_LINE:
		{
			list_t *copy = list_copy_deep(ynk->list, NULL);
			list_t *copy_tail = list_tail(copy);

			if(prepend){
				if(*py == 0){
					list_t *head = *phead;

					copy->prev = NULL;
					*phead = copy;

					copy_tail->next = head;
					if(head)
						head->prev = copy_tail;
				}else{
					list_t *before = list_seek(*phead, *py - 1, true);
					list_t *after = before->next;

					copy->prev = before;
					before->next = copy;

					copy_tail->next = after;
					if(after)
						after->prev = copy_tail;
				}
			}else{
				list_t *const head = *list_seekp(phead, *py, true);
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
			list_t *head = *list_seekp(phead, *py, true);
			assert(head);
			int y = 0;
			for(const list_t *l = ynk->list;
			    l; l = l->next, y++)
			{
				for(unsigned i = 0; i < l->len_line; i++)
					list_inschar(head,
							&(int){ *px + !prepend + i },
							&(int){ y },
							l->line[i], /* force spaces to fill in for now */' ');
			}

			if(!prepend)
				++*px;
			break;
		}
	}
}
