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

list_t *list_seek(list_t *l, int y, int creat);

void    list_free(list_t *);

void list_inschar(list_t **, int *x, int *y, char ch);
void list_delchar(list_t *, int *x, int *y);

void list_delregion(list_t **pl, const region_t *);
void list_joinregion(list_t **pl, const region_t *);

void list_replace_at(list_t *, int *px, int *py, char *with);

int list_filter(
		list_t **pl, const region_t *,
		const char *cmd);

void list_insline(list_t **, int *x, int *y, int dir);

int list_count(list_t *);

#define isnewline(ch) ((ch) == '\n' || (ch) == '\r')

#endif
