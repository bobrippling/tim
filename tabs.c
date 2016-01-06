#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "window.h"
#include "windows.h"
#include "main.h"
#include "tab.h"
#include "tabs.h"

#include "ui.h"

#include "tabs.h"

static tab *tab_sel, *tab_first;

tab *tabs_cur()
{
	return tab_sel;
}

void tabs_set_cur(tab *new)
{
	tab_sel = new;
}

tab *tabs_first(void)
{
	return tab_first;
}

void tabs_set_first(tab *t)
{
	tab_first = t;
}

bool tabs_single(void)
{
	return tabs_count() == 1;
}

unsigned tabs_count(void)
{
	unsigned n = 0;

	for(tab *t = tabs_first(); t; t = t->next, n++);

	return n;
}

static void win_add_splitright(window *cur, window *new)
{
	window_add_neighbour(cur, neighbour_right, new);
}

static void win_add_splittop(window *cur, window *new)
{
	window_add_neighbour(cur, neighbour_down, new);
}

static void load_argv(void onload(window *cur, window *new))
{
	window *prev_win = windows_cur();
	char *next;

	while((next = args_next_fname(true))){
		buffer_t *buf;
		const char *err;

		buffer_new_fname(&buf, next, &err);

		window *w = window_new(buf);

		buffer_release(buf);

		onload(prev_win, w);

		prev_win = w;
	}
}

void tabs_init(enum init_args init_args, unsigned off)
{
	char *next = args_next_fname(true);

	if(next){
		const char *err = NULL;
		buffer_t *buf;

		if(!strcmp(next, "-")){
			buf = buffer_new_file_nofind(stdin);
			buf->modified = true; /* editing a stream */
			/* no filename */

			if(ferror(stdin))
				err = strerror(errno);

			freopen("/dev/tty", "r", stdin);
			/* if we can't open it we'll exit soon anyway */

		}else{
			buffer_new_fname(&buf, next, &err);
		}

		if(err)
			ui_err("\"%s\": %s", next, err);

		tab *tab = tab_new(window_new(buf));
		buffer_release(buf);

		tabs_set_cur(tab);

		switch(init_args){
			case INIT_NONE:
				break;

			case INIT_VALL:
				load_argv(win_add_splitright);
				break;

			case INIT_HALL:
				load_argv(win_add_splittop);
				break;
		}

	}else{
		buffer_t *empty = buffer_new();

		window *win = window_new(empty);
		buffer_release(empty);

		tab *tab = tab_new(win);

		tabs_set_cur(tab);
	}

	tabs_set_first(tabs_cur());

	windows_cur()->ui_pos->y = off;
}

void tabs_term(void)
{
	if(tabs_cur()){
		tab_free(tabs_cur());
		tabs_set_cur(NULL);
	}
}
