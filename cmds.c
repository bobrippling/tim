#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cmds.h"
#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "ui.h"
#include "buffers.h"
#include "external.h"
#include "mem.h"

enum
{
	CMD_FAILURE,
	CMD_SUCCESS,
};

int c_q(int argc, char **argv, bool force)
{
	if(argc != 1){
		ui_status("usage: %s", *argv);
		return CMD_FAILURE;
	}

	if(!force && buffers_cur()->modified){
		ui_status("buffer modified");
		return CMD_FAILURE;
	}

	ui_run = UI_EXIT_0;

	return CMD_SUCCESS;
}

int c_cq(int argc, char **argv, bool force)
{
	if(argc != 1){
		ui_status("usage: %s", *argv);
		return CMD_FAILURE;
	}

	/* no buffer checks */
	ui_run = UI_EXIT_1;

	return CMD_SUCCESS;
}

int c_w(int argc, char **argv, bool force)
{
	buffer_t *const buf = buffers_cur();

	bool newfname = false;
	if(argc == 2){
		const char *old = buffer_fname(buf);

		newfname = !old || strcmp(old, argv[1]);

		buffer_set_fname(buf, argv[1]);
	}else if(argc != 1){
		ui_status("usage: %s filename", *argv);
		return CMD_FAILURE;
	}

	const char *fname = buffer_fname(buf);
	if(!fname){
		ui_status("no filename");
		return CMD_FAILURE;
	}

	struct stat st;
	if(!force){
		if(stat(fname, &st) == 0){
			if(newfname && access(fname, F_OK) == 0){
				ui_status("file already exists");
				return CMD_FAILURE;
			}

			if(st.st_mtime > buf->mtime){
				ui_status("file modified externally");
				return CMD_FAILURE;
			}
		}else if(errno != ENOENT){
			ui_status("stat(%s): %s", buffer_shortfname(fname), strerror(errno));
			return CMD_FAILURE;
		}
	}

	FILE *f = fopen(fname, "w");

	if(!f){
got_err:
		ui_status("%s: %s", buffer_shortfname(fname), strerror(errno));
		if(f)
			fclose(f);
		return CMD_FAILURE;
	}

	buffer_write_file(buf, -1, f, buf->eol);

	if(fclose(f)){
		f = NULL;
		goto got_err;
	}

	ui_status("written to \"%s\"", buffer_shortfname(fname));

	return CMD_SUCCESS;
}

int c_x(int argc, char **argv, bool force)
{
	return c_w(argc, argv, false) && c_q(argc, argv, false);
}

int c_e(int argc, char **argv, bool force)
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

	buffer_t *const buf = buffers_cur();
	if(!force && buf->modified){
		ui_status("buffer modified");
		return CMD_FAILURE;
	}

	if(!buffer_replace_fname(buf, fname)){
		buffer_t *b = buffer_new(); /* FIXME: use buffer_new_fname() instead? */
		buffers_set_cur(b);
		ui_status("%s: %s", buffer_shortfname(fname), strerror(errno));
	}else{
		ui_status("%s: loaded", buffer_shortfname(fname));
		buffers_cur()->modified = false;
	}

	buffer_set_fname(buffers_cur(), fname);

	ui_redraw();
	ui_cur_changed();

	return CMD_SUCCESS;
}

static
int c_split(enum buffer_neighbour ne, int argc, char **argv, bool force)
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
			ui_status("%s: %s", buffer_shortfname(argv[1]), strerror(errno));
	}else{
		b = buffer_new();
	}

	buffer_add_neighbour(buffers_cur(), ne, b);
	ui_redraw();
	ui_cur_changed();

	return CMD_SUCCESS;
}

int c_vs(int argc, char **argv, bool force)
{
	return c_split(BUF_RIGHT, argc, argv, force);
}

int c_sp(int argc, char **argv, bool force)
{
	return c_split(BUF_DOWN, argc, argv, force);
}

int c_run(int argc, char **argv, bool force)
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
