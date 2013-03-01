#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "cmds.h"
#include "ui.h"

#include "pos.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"
#include "external.h"
#include "mem.h"

enum
{
	CMD_SUCCESS,
	CMD_FAILURE
};

int c_q(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	ui_running = 0;

	return CMD_SUCCESS;
}

int c_w(int argc, char **argv)
{
	FILE *f;
	list_t *l;
	const char *fname;

	if(argc == 2){
		buffer_set_fname(buffers_cur(), argv[1]);
	}else if(argc != 1){
		ui_status("usage: %s filename", *argv);
		return CMD_FAILURE;
	}

	fname = buffer_fname(buffers_cur());
	if(!fname){
		ui_status("no filename");
		return CMD_FAILURE;
	}

	f = fopen(fname, "w");

	if(!f){
got_err:
		ui_status("%s: %s", fname, strerror(errno));
		if(f)
			fclose(f);
		return CMD_FAILURE;
	}

	for(l = buffers_cur()->head; l; l = l->next)
		if(fwrite(l->line, 1, l->len_line, f) != l->len_line || (l->next ? fputc('\n', f) == EOF : 0))
			goto got_err;

	if(fclose(f)){
		f = NULL;
		goto got_err;
	}

	ui_status("written to \"%s\"", fname);

	return CMD_SUCCESS;
}

int c_x(int argc, char **argv)
{
	return c_w(argc, argv) == CMD_SUCCESS && c_q(0, NULL) == CMD_SUCCESS ? CMD_SUCCESS : CMD_FAILURE;
}

int c_e(int argc, char **argv)
{
	const char *fname;

	if(argc == 1){
		fname = buffer_fname(buffers_cur());
		if(!fname){
			ui_status("no filename");
			return CMD_FAILURE;
		}
	}else if(argc == 2){
		fname = argv[1];
	}else{
		ui_status("usage: %s [filename]", *argv);
		return CMD_FAILURE;
	}

	if(!buffer_replace_fname(buffers_cur(), fname)){
		buffer_t *b = buffer_new(); /* FIXME: use buffer_new_fname() instead? */
		buffers_set_cur(b);
		ui_status("%s: %s", fname, strerror(errno));
	}else{
		ui_status("%s: loaded", fname);
	}

	buffer_set_fname(buffers_cur(), fname);

	ui_redraw();
	ui_cur_changed();

	return CMD_SUCCESS;
}

static
int c_split(enum buffer_neighbour ne, int argc, char **argv)
{
	buffer_t *b;

	if(argc > 2){
		ui_status("usage: %s [filename]", *argv);
		return CMD_FAILURE;
	}

	if(argc > 1){
		int err;
		buffer_new_fname(&b, argv[1], &err);

		if(err)
			ui_status("%s: %s", argv[1], strerror(errno));
	}else{
		b = buffer_new();
	}

	buffer_add_neighbour(buffers_cur(), ne, b);
	ui_redraw();
	ui_cur_changed();

	return CMD_SUCCESS;
}

int c_vs(int argc, char **argv)
{
	return c_split(BUF_RIGHT, argc, argv);
}

int c_sp(int argc, char **argv)
{
	return c_split(BUF_DOWN, argc, argv);
}

int c_run(int argc, char **argv)
{
	if(argc == 1){
		shellout(NULL);
	}else{
		char *cmd, *p;
		int len, i;

		for(len = 0, i = 1; i < argc; i++)
			len += strlen(argv[i]) + 1;

		p = cmd = umalloc(len + 1);
		*cmd = '\0';

		for(len = 0, i = 1; i < argc; i++)
			p += sprintf(p, "%s ", argv[i]);

		shellout(cmd);

		free(cmd);
	}

	return CMD_SUCCESS;
}
