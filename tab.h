#ifndef TAB_H
#define TAB_H

#include <stdbool.h>

typedef struct tab tab;

struct tab
{
	struct window *win; /* can get to other tabs via win */
	tab *next;
};

tab *tab_new(struct window *);
void tab_free(tab *);

void tab_set_focus(tab *, struct window *);
void tab_evict(tab *);

tab *tab_next(tab *);

struct buffer;
bool tab_contains_buffer(tab *, struct buffer *);

#endif
