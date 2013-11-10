#ifndef LIST_H
#define LIST_H

typedef struct list_ list_t;

struct list_
{
	char *line;
	size_t len_line, len_malloc;

	list_t *next, *prev;
};

list_t *list_new(list_t *prev);
list_t *list_new_file(FILE *, bool *eol);
int list_write_file(list_t *l, int n, FILE *f, bool eol);

list_t *list_copy_deep(const list_t *const l, list_t *prev);
list_t *list_append(list_t *accum, list_t *new);

list_t **list_seekp(list_t **pl, int y, bool creat);
list_t *list_seek(list_t *l, int y, bool creat);

void    list_free(list_t *);

void list_inschar(list_t *, int *x, int *y, char ch);
void list_delchar(list_t *, int *x, int *y);

list_t *list_delregion(list_t **pl, const region_t *);
void list_joinregion(list_t **pl, const region_t *);

void list_replace_at(list_t *, int *px, int *py, char *with);

int list_filter(
		list_t **pl, const region_t *,
		const char *cmd);

void list_iter_region(
		list_t *, const region_t *,
		void fn(char *, void *), void *ctx);

void list_insline(list_t **, int *x, int *y, int dir);

int list_count(list_t *);
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

#endif
