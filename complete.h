#ifndef COMPLETE_H
#define COMPLETE_H

#include <stdbool.h>

struct complete_ctx
{
	char *current;
	size_t current_len;
	struct hash *ents;
};

bool complete_init(struct complete_ctx *, char *line, unsigned len, int x);
void complete_gather(char *line, void *ctx);
void complete_teardown(struct complete_ctx *);
void complete_filter(struct complete_ctx *, int newch);

bool complete_1_ishidden(void *);
char *complete_1_getstr(void *);

#endif
