#ifndef LIST_H
#define LIST_H

typedef struct list
{
	char *line;
	int len_line, len_malloc;

	struct list *next;
} list_t;

list_t *list_new(void);
list_t *list_new_file(FILE *);

list_t *list_seek(list_t *l, int y);

void    list_free(list_t *);

void    list_inschar(list_t *, int *x, int *y, char ch);
void    list_delchar(list_t *, int *x, int *y);

#endif
