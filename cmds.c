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
#include "io.h"
#include "parse_cmd.h"
#include "str.h"

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
		&& c_q(1, (char *[]){ *argv }, false, NULL);
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

	int ch = io_getch(IO_NOMAP, NULL);
	if(!isnewline(ch))
		io_ungetch(ch);
	ui_redraw();
	ui_cur_changed();

	return true;
}

struct g_ctx
{
	bool g_inverse;
	char *match;
	buffer_t *buf;

	const cmd_t *cmd;
	int argc;
	char **argv;
	bool force;
	struct range *range;
};

static void g_exec(char *line, size_t len, int y, void *c)
{
	struct g_ctx *ctx = c;

	if(!!tim_strstr(line, len, ctx->match) == ctx->g_inverse)
		return;

	*ctx->buf->ui_pos = (point_t){ .y = y };

	cmd_dispatch(ctx->cmd,
			ctx->argc, ctx->argv,
			ctx->force, ctx->range);
}

bool c_g(char *cmd, char *gcmd, bool inverse, struct range *range)
{
	const char reg_sep = *gcmd;
	struct
	{
		char *start, *end;
	} regex = { .start = gcmd + 1 };

	for(regex.end = regex.start; *regex.end; regex.end++)
		if(*regex.end == '\\')
			regex.end++;
		else if(*regex.end == reg_sep)
			break;

	if(*regex.end != reg_sep){
		ui_err("g%c: no terminating character (%c)", reg_sep, reg_sep);
		return false;
	}
	*regex.end = '\0';

	buffer_t *const b = buffers_cur();

	char *subcmd = regex.end + 1;

	struct range sub_range;
	struct g_ctx ctx = {
		.g_inverse = inverse,
		.match = regex.start,
		.range = &sub_range,
		.buf = b,
	};


	if(!parse_ranged_cmd(
			subcmd, &ctx.cmd,
			&ctx.argv, &ctx.argc,
			&ctx.force, &ctx.range))
	{
		ui_err("unknown command: %s", subcmd);
		goto out;
	}

	struct range rng_all;
	if(!range){
		rng_all.start = 0;
		rng_all.end = buffer_nlines(b);
		range = &rng_all;
	}

	const point_t orig_pos = *b->ui_pos;

	list_iter_region(
			b->head,
			&(struct region){
				.type = REGION_LINE,
				.begin.y = range->start,
				.end.y = range->end
			},
			/*evalnl:*/true,
			g_exec, &ctx);

	*b->ui_pos = orig_pos;

out:
	free_argv(ctx.argv, ctx.argc);

	return true;
}

bool cmd_dispatch(
		const cmd_t *cmd_f,
		int argc, char **argv,
		bool force, struct range *range)
{
		return cmd_f->single_arg
			? cmd_f->f_arg1(argv[0], argv[1], force, range)
			: cmd_f->f_argv(argc, argv, force, range);
}
