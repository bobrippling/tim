#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"
#include "mem.h"
#include "ui.h"

static buffer_t *buf_sel;
static char **remaining_fnames;

char *buffers_next_fname(bool pop)
{
	char *fname = remaining_fnames ? *remaining_fnames : NULL;
	if(pop && fname)
		remaining_fnames++;
	return fname;
}

buffer_t *buffers_cur()
{
	return buf_sel;
}

void buffers_set_cur(buffer_t *b)
{
	buf_sel = b;
}

static void buf_add_splitright(buffer_t *cur, buffer_t *new)
{
	buffer_add_neighbour(cur, true, new);
}

static void buf_add_splittop(buffer_t *cur, buffer_t *new)
{
	buffer_add_neighbour(cur, false, new);
}

static void load_argv(int argc, char **argv, void onload(buffer_t *cur, buffer_t *new))
{
	buffer_t *prev_buf = buf_sel;

	for(int i = 1; i < argc; i++){
		buffer_t *b;

		int err;
		buffer_new_fname(&b, argv[i], &err);
		/* FIXME: ignore errors? */

		onload(prev_buf, b);

		prev_buf = b;
	}
}

void buffers_init(int argc, char **argv, enum buffer_init_args init_args, unsigned off)
{
	if(argc){
		int err;
		buffer_new_fname(&buf_sel, argv[0], &err);
		if(err)
			ui_err("\"%s\": %s", buffer_shortfname(argv[0]), strerror(errno));

		switch(init_args){
			case BUF_VALL:
				load_argv(argc, argv, buf_add_splitright);
				break;

			case BUF_HALL:
				load_argv(argc, argv, buf_add_splittop);
				break;

			default:
				/* stash argv */
				remaining_fnames = argv + 1;
				break;
		}

	}else{
		buf_sel = buffer_new();
	}

	if(off)
		buf_sel->ui_pos->y = off;
}

void buffers_term(void)
{
	if(buf_sel)
		buffer_free(buf_sel), buf_sel = NULL;
}
