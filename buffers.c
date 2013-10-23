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

static buffer_t   *buf_sel;

static char      **buf_list;
static int         buf_c;

buffer_t *buffers_cur()
{
	return buf_sel;
}

void buffers_set_cur(buffer_t *b)
{
	/* FIXME: free old */
	buf_sel = b;
}

void buffers_init(int argc, char **argv, enum buffer_init_args a, unsigned off)
{
	int i;

	if(argc){
		buf_c = argc;
		buf_list = umalloc(argc * sizeof *buf_list);
		for(i = 0; i < argc; i++)
			buf_list[i] = ustrdup(argv[i]);

		int err;
		buffer_new_fname(&buf_sel, argv[0], &err);
		if(err)
			ui_status("\"%s\": %s", buffer_shortfname(argv[0]), strerror(errno));

		enum buffer_neighbour dir;
		switch(a){
			case BUF_VALL: dir = BUF_RIGHT; break;
			case BUF_HALL: dir = BUF_DOWN;  break;
			default:
				goto fin;
		}

		buffer_t *prev_buf = buf_sel;
		for(i = 1; i < argc; i++){
			buffer_t *b;
			int err;

			buffer_new_fname(&b, argv[i], &err);
			/* FIXME: ignore errors? */

			buffer_add_neighbour(prev_buf, dir, b);

			prev_buf = b;
		}
	}else{
		buf_sel = buffer_new();
	}

fin:
	if(off)
		buf_sel->ui_pos->y = off;
}

void buffers_term(void)
{
	if(buf_sel)
		buffer_free(buf_sel), buf_sel = NULL;
}
