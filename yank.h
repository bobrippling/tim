#ifndef YANK_H
#define YANK_H

typedef struct yank yank;

yank *yank_new(list_t *, enum region_type as);

yank *yank_push(yank *);
const yank *yank_top(void);
void yank_free(const yank *);

void yank_put_in_list(
		const yank *ynk, list_t **head,
		bool prepend,
		int *y, int *x);

#endif
