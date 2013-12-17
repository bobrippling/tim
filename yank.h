#ifndef YANK_H
#define YANK_H

typedef struct yank yank;

yank *yank_new(list_t *, enum region_type as);

void yank_push(yank *);
const yank *yank_top(void);

void yank_put_in_list(
		const yank *ynk, list_t **head,
		bool prepend,
		int *y, int *x);

#endif
