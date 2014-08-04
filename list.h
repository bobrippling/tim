#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

typedef struct list_ list_t;

struct list_
{
	char *line;
	size_t len_line, len_malloc;

	list_t *next, *prev;

	int flag;
};

list_t *list_new(list_t *prev);
list_t *list_new_file(FILE *, bool *eol);
int list_write_file(list_t *l, int n, FILE *f, bool eol);

list_t *list_copy_deep(const list_t *const l, list_t *prev);
list_t *list_append(list_t *accum, list_t *at, list_t *new);

list_t **list_seekp(list_t **pl, int y, bool creat);
list_t *list_seek(list_t *l, int y, bool creat);

void    list_free(list_t *);

void list_inschar(list_t *, int *x, int *y, char ch);
void list_delchar(list_t *, int *x, int *y);

list_t *list_delregion(list_t **pl, const region_t *);

/* *pl isn't changed - ABI compat with list_delregion */
void list_joinregion(list_t **pl, const region_t *);

int list_filter(
		list_t **pl, const region_t *,
		const char *cmd);

/* return false to stop */
typedef bool list_iter_f(char *, list_t *, int y, void *);

enum list_iter_flags
{
	LIST_ITER_EVAL_NL = 1 << 0,
	LIST_ITER_WHOLE_LINE = 1 << 1,
};

void list_iter_region(
		list_t *, const region_t *,
		enum list_iter_flags,
		const bool create,
		list_iter_f fn, void *ctx);

void list_insline(list_t **, int *x, int *y, int dir);
int list_evalnewlines1(list_t *l);

int list_count(const list_t *);
list_t *list_tail(list_t *);

#define isnewline(ch) ((ch) == '\n' || (ch) == '\r')

list_t *list_advance_y(
		list_t *l, const int dir,
		int *py, int *px)
	__attribute__((nonnull(1, 3)));

list_t *list_advance_x(
		list_t *l, const int dir,
		int *py, int *px)
	__attribute__((nonnull));

list_t *list_last(list_t *, int *py);

void list_clear_flag(list_t *);
struct range;
void list_flag_range(list_t *, const struct range *, int);
list_t *list_flagfind(list_t *, int flag, int *py);

#endif
