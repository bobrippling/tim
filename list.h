#ifndef LIST_H
#define LIST_H

typedef struct list
{
	char *line;
	size_t len_line, len_malloc;

	struct list *next;
} list_t;

list_t *list_new(void);
list_t *list_new_file(FILE *);

list_t *list_seek(list_t *l, int y, int creat);

void    list_free(list_t *);

void    list_inschar(list_t *, int *x, int *y, char ch);
void    list_delchar(list_t *, int *x, int *y);
void    list_delbetween(list_t **pl,
                        point_t const *from,
                        point_t const *to,
                        int linewise);

void    list_insline(list_t **, int *x, int *y, int dir);

int list_count(list_t *);

#endif
