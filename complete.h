#ifndef COMPLETE_H
#define COMPLETE_H

#include <stdbool.h>

struct complete_ctx
{
	struct hash *ents;
	char *current_word;
	size_t current_word_len;
	size_t recalc_len;
	char *(*get_word)(char *, int);
	char *(*get_end)(char *, char *, char *, struct complete_ctx *);
};

bool complete_init(
		struct complete_ctx *ctx,
		char *line, unsigned len, int x,
		char *getter_word(char *line, int x),
		char *getter_end(char *, char *, char *, struct complete_ctx *));

void complete_gather_all(struct complete_ctx *);

void complete_gather(char *line, size_t, void *ctx);
void complete_teardown(struct complete_ctx *);

void complete_to_longest_common(
		struct complete_ctx *ctx,
		size_t *const n_to_insert);

void complete_filter(
		struct complete_ctx *, int newch,
		bool *const cancel, bool *const recalc);

void *complete_hash_ent(struct hash *, size_t sel);

bool complete_1_ishidden(const void *);
bool complete_1_isvisible(const void *);
char *complete_1_getstr(const void *);

#ifdef RECT_H
void complete_draw_menu(struct hash *, int sel, point_t const *);
#endif

/* line vs. word functions */
char *complete_get_end_of_word(
		char *line, char *found, char *line_end,
		struct complete_ctx *ctx);

char *complete_get_end_of_line(
		char *line, char *found, char *line_end,
		struct complete_ctx *ctx);

#endif
