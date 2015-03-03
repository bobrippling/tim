#ifndef TAB_H
#define TAB_H

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

#endif
