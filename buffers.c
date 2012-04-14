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
static buffer_t  **buf_vis;

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

void buffers_init(int argc, char **argv)
{
	int i;

	if(argc){
		buf_c = argc;
		buf_list = umalloc(argc * sizeof *buf_list);
		for(i = 0; i < argc; i++)
			buf_list[i] = ustrdup(argv[i]);

		buf_sel = buffer_new_fname(argv[0]);
		if(!buf_sel)
			ui_status("\"%s\": %s", argv[0], strerror(errno));
	}

	if(!buf_sel)
		buf_sel = buffer_new();

	buf_vis = umalloc(2);
	buf_vis[0] = buf_sel;
	buf_vis[1] = NULL;
}

buffer_t **buffers_vis()
{
	return buf_vis;
}
