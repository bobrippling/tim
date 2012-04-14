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
	ui_running = 0;
}

void c_w(int argc, char **argv)
{
	FILE *f = fopen("out", "w");
	if(f){
		list_t *l;
		for(l = buffers_cur()->head; l; l = l->next){
			fwrite(l->line, 1, l->len_line, f);
			fputc('\n', f);
		}
		fclose(f);

		ui_status("written to \"out\"");
	}
}

void c_split(enum buffer_neighbour ne, int argc, char **argv)
{
	buffer_t *b;

	if(argc != 2){
		ui_status("usage: %s filename", *argv);
		return;
	}

	b = buffer_new_fname(argv[1]);

	if(!b){
		ui_status("%s: %s", argv[1], strerror(errno));
		return;
	}

	buffer_add_neighbour(buffers_cur(), ne, b);
	ui_redraw();
}

void c_vs(int argc, char **argv)
{
	c_split(BUF_RIGHT, argc, argv);
}

void c_sp(int argc, char **argv)
{
	c_split(BUF_DOWN, argc, argv);
}
