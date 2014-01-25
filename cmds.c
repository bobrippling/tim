#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "range.h"
#include "cmds.h"
#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "ui.h"
#include "buffers.h"
#include "external.h"
#include "mem.h"

#define RANGE_NO()                       \
	if(range){                             \
		ui_err(":%s doesn't support ranges", \
				*argv);                          \
		return false;                        \
	}

#define RANGE_TODO()                       \
	if(range){                               \
		ui_err("TODO: range with :%s", *argv); \
		return false;                          \
	}

bool c_q(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	if(argc != 1){
		ui_err("usage: %s", *argv);
		return false;
	}

	if(!force && buffers_cur()->modified){
		ui_err("buffer modified");
		return false;
	}

	ui_run = UI_EXIT_0;

	return true;
}

bool c_cq(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	if(argc != 1){
		ui_err("usage: %s", *argv);
		return false;
	}

	/* no buffer checks */
	ui_run = UI_EXIT_1;

	return true;
}

bool c_w(int argc, char **argv, bool force, struct range *range)
{
	RANGE_TODO();

	buffer_t *const buf = buffers_cur();

	bool newfname = false;
	if(argc == 2){
		const char *old = buffer_fname(buf);

		newfname = !old || strcmp(old, argv[1]);

		buffer_set_fname(buf, argv[1]);
	}else if(argc != 1){
		ui_err("usage: %s filename", *argv);
		return false;
	}

	const char *fname = buffer_fname(buf);
	if(!fname){
		ui_err("no filename");
		return false;
	}

	struct stat st;
	if(!force){
		if(stat(fname, &st) == 0){
			if(newfname && access(fname, F_OK) == 0){
				ui_err("file already exists");
				return false;
			}

			if(st.st_mtime > buf->mtime){
				ui_err("file modified externally");
				return false;
			}
		}else if(errno != ENOENT){
			ui_err("stat(%s): %s", buffer_shortfname(fname), strerror(errno));
			return false;
		}
	}

	FILE *f = fopen(fname, "w");

	if(!f){
got_err:
		ui_err("%s: %s", buffer_shortfname(fname), strerror(errno));
		if(f)
			fclose(f);
		return false;
	}

	buffer_write_file(buf, -1, f, buf->eol);

	if(fclose(f)){
		f = NULL;
		goto got_err;
	}

	ui_status("written to \"%s\"", buffer_shortfname(fname));

	return true;
}

bool c_x(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	return c_w(argc, argv, false, range)
		&& c_q(argc, argv, false, NULL);
}

bool c_e(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	const char *fname;

	if(argc == 1){
		fname = buffer_fname(buffers_cur());
		if(!fname){
			ui_err("no filename");
			return false;
		}
	}else if(argc == 2){
		fname = argv[1];
	}else{
		ui_err("usage: %s [filename]", *argv);
		return false;
	}

	buffer_t *const buf = buffers_cur();
	if(!force && buf->modified){
		ui_err("buffer modified");
		return false;
	}

	if(!buffer_replace_fname(buf, fname)){
		buffer_t *b = buffer_new(); /* FIXME: use buffer_new_fname() instead? */
		buffers_set_cur(b);
		ui_err("%s: %s", buffer_shortfname(fname), strerror(errno));
	}else{
		ui_status("%s: loaded", buffer_shortfname(fname));
		buffers_cur()->modified = false;
	}

	buffer_set_fname(buffers_cur(), fname);

	ui_redraw();
	ui_cur_changed();

	return true;
}

static
bool c_split(enum buffer_neighbour ne, int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	buffer_t *b;

	if(argc > 2){
		ui_err("usage: %s [filename]", *argv);
		return false;
	}

	if(argc > 1){
		int err;
		buffer_new_fname(&b, argv[1], &err);

		if(err)
			ui_err("%s: %s", buffer_shortfname(argv[1]), strerror(errno));
	}else{
		b = buffer_new();
	}

	buffer_add_neighbour(buffers_cur(), ne, b);
	ui_redraw();
	ui_cur_changed();

	return true;
}

bool c_vs(int argc, char **argv, bool force, struct range *range)
{
	return c_split(BUF_RIGHT, argc, argv, force, range);
}

bool c_sp(int argc, char **argv, bool force, struct range *range)
{
	return c_split(BUF_DOWN, argc, argv, force, range);
}

bool c_run(int argc, char **argv, bool force, struct range *range)
{
	RANGE_TODO();

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

	return true;
}

bool c_p(int argc, char **argv, bool force, struct range *range)
{
	if(argc != 1){
		ui_err("Usage: %s", *argv);
		return false;
	}

	buffer_t *const b = buffers_cur();

	struct range rng;
	if(!range){
		range = &rng;
		rng.start = rng.end = b->ui_pos->y;
	}

	list_t *l = list_seek(b->head, range->start, false);
	for(int i = range->start; i <= range->end; i++){
		ui_print(l ? l->line : "", l ? l->len_line : 0);
		if(l)
			l = l->next;
	}

	return true;
}
