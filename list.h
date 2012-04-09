#ifndef LIST_H
#define LIST_H

typedef struct list
{
	char *line;
	int len_line, len_malloc;

	struct list *next;
} list_t;

list_t *list_new(void);
void    list_inschar(list_t *, int x, int y, char ch);

#endif
