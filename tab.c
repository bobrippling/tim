#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tab.h"
#include "tabs.h"

#include "mem.h"

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "window.h"
#include "windows.h"

tab *tab_new(window *win)
{
	tab *t = umalloc(sizeof *t);
	t->win = win;
	t->next = NULL;
	return t;
}

void tab_free(tab *t)
{
	if(t->win){
		window_free(t->win);
		t->win = NULL;
	}

	if(t == tabs_first())
		tabs_set_first(t->next);

	free(t);
}

void tab_set_focus(tab *tab, window *focus)
{
	tab->win = focus;
}

void tab_evict(tab *t)
{
	tab *i;

	for(i = tabs_first(); i; i = i->next){
		if(i->next == t){
			i->next = t->next;
			break;
		}
	}

	t->next = NULL;
}

tab *tab_next(tab *t)
{
	return t->next ? t->next : tabs_first();
}

bool tab_contains_buffer(tab *t, buffer_t *b)
{
	window *win;
	ITER_WINDOWS_FROM(win, t->win)
		if(win->buf == b)
			return true;

	return false;
}
