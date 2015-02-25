#ifndef COMPLETE_H
#define COMPLETE_H

#include <stdbool.h>

struct complete_ctx
{
	struct hash *ents;
	char *current_word;
	size_t current_word_len;
	size_t recalc_len;
};

bool complete_init(struct complete_ctx *, char *line, unsigned len, int x);
void complete_gather(char *line, size_t, void *ctx);
void complete_teardown(struct complete_ctx *);

void complete_filter(
		struct complete_ctx *, int newch,
		bool *const cancel, bool *const recalc);

void *complete_hash_ent(struct hash *, size_t sel);

bool complete_1_ishidden(const void *);
bool complete_1_isvisible(const void *);
char *complete_1_getstr(const void *);

#endif
