#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "pos.h"
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
	buf_sel = b;
}

void buffers_init(int argc, char **argv, enum buffer_init_args a)
{
	int i;

	if(argc){
		enum buffer_neighbour dir;
		buffer_t *prev_buf;

		buf_c = argc;
		buf_list = umalloc(argc * sizeof *buf_list);
		for(i = 0; i < argc; i++)
			buf_list[i] = ustrdup(argv[i]);

		buf_sel = buffer_new_fname(argv[0]);
		if(!buf_sel){
			ui_status("\"%s\": %s", argv[0], strerror(errno));
			buf_sel = buffer_new();
		}

		switch(a){
			case BUF_VALL: dir = BUF_RIGHT; break;
			case BUF_HALL: dir = BUF_DOWN;  break;
			default:
				goto fin;
		}

		prev_buf = buf_sel;

		for(i = 1; i < argc; i++){
			buffer_t *b = buffer_new_fname(argv[i]);
			if(!b)
				b = buffer_new();

			buffer_add_neighbour(prev_buf, dir, b);

			prev_buf = b;
		}
	}else{
		buf_sel = buffer_new();
	}

fin:;
}
