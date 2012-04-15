#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cmds.h"
#include "ui.h"

#include "list.h"
#include "pos.h"
#include "buffer.h"
#include "buffers.h"

void c_q(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	ui_running = 0;
}

void c_w(int argc, char **argv)
{
	FILE *f;
	list_t *l;

	if(argc != 2){
		ui_status("usage: %s filename", *argv);
		return;
	}

	f = fopen(argv[1], "w");

	if(!f){
got_err:
		ui_status("%s: %s", argv[1], strerror(errno));
		if(f)
			fclose(f);
		return;
	}

	for(l = buffers_cur()->head; l; l = l->next){
		if(fwrite(l->line, 1, l->len_line, f) != l->len_line || fputc('\n', f) == EOF)
			goto got_err;
	}

	if(fclose(f))
		goto got_err;
	else
		ui_status("written to \"%s\"", argv[1]);
}

void c_split(enum buffer_neighbour ne, int argc, char **argv)
{
	buffer_t *b;

	if(argc > 2){
		ui_status("usage: %s filename", *argv);
		return;
	}

	if(argc > 1){
		b = buffer_new_fname(argv[1]);

		if(!b){
			ui_status("%s: %s", argv[1], strerror(errno));
			return;
		}
	}else{
		b = buffer_new();
	}

	buffer_add_neighbour(buffers_cur(), ne, b);
	ui_redraw();
	ui_cur_changed();
}

void c_vs(int argc, char **argv)
{
	c_split(BUF_RIGHT, argc, argv);
}

void c_sp(int argc, char **argv)
{
	c_split(BUF_DOWN, argc, argv);
}
