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

#include "ui.h"

static window *win_sel;

window *windows_cur()
{
	return win_sel;
}

void windows_set_cur(window *new)
{
	win_sel = new;
}

static void win_add_splitright(window *cur, window *new)
{
	window_add_neighbour(cur, neighbour_right, new);
}

static void win_add_splittop(window *cur, window *new)
{
	window_add_neighbour(cur, neighbour_down, new);
}

static void load_argv(
		int argc, char **argv,
		void onload(window *cur, window *new))
{
	window *prev_win = win_sel;

	for(int i = 1; i < argc; i++){
		buffer_t *buf;
		const char *err;

		buffer_new_fname(&buf, argv[i], &err);

		window *w = window_new(buf);

		buffer_release(buf);

		onload(prev_win, w);

		prev_win = w;
	}
}

void windows_init(
		int argc, char **argv,
		enum windows_init_args init_args, unsigned off)
{
	if(argc){
		const char *err = NULL;
		buffer_t *buf;

		if(!strcmp(argv[0], "-")){
			buf = buffer_new_file_nofind(stdin);
			buf->modified = true; /* editing a stream */
			/* no filename */

			if(ferror(stdin))
				err = strerror(errno);

			freopen("/dev/tty", "r", stdin);
			/* if we can't open it we'll exit soon anyway */

		}else{
			buffer_new_fname(&buf, argv[0], &err);
		}

		if(err)
			ui_err("\"%s\": %s", argv[0], err);

		win_sel = window_new(buf);
		buffer_release(buf);

		switch(init_args){
			case WIN_VALL:
				load_argv(argc, argv, win_add_splitright);
				break;

			case WIN_HALL:
				load_argv(argc, argv, win_add_splittop);
				break;

			default:
				/* stash argv */
				remaining_fnames = argv + 1;
				break;
		}

	}else{
		buffer_t *empty = buffer_new();
		win_sel = window_new(empty);
		buffer_release(empty);
	}

	if(off)
		win_sel->ui_pos->y = off;
}

void windows_term(void)
{
	if(win_sel)
		window_free(win_sel), win_sel = NULL;
}
