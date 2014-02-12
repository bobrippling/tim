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

#define RANGE_DEFAULT(range, store, y) \
	if(!range){                          \
		store.start = store.end = y;       \
		range = &store;                    \
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
	RANGE_DEFAULT(range, rng, b->ui_pos->y);

	list_t *l = list_seek(b->head, range->start, false);
	for(int i = range->start; i <= range->end; i++){
		ui_print(l ? l->line : "", l ? l->len_line : 0);
		if(l)
			l = l->next;
	}

	ui_want_return();

	return true;
}

static void command_bufaction(
		struct range *range, buffer_action_f *buf_fn)
{
	buffer_t *const b = buffers_cur();

	struct range range_store;
	RANGE_DEFAULT(range, range_store, b->ui_pos->y);

	buf_fn(
			b,
			&(region_t){
				.begin.y = range->start,
				.end.y = range->end + 1,
				.type = REGION_LINE
			},
			&(point_t){});

	*b->ui_pos = (point_t){ .y = range->end };

	ui_redraw();
	ui_cur_changed();
}

bool c_d(int argc, char **argv, bool force, struct range *range)
{
	if(argc != 1){
		ui_err("Usage: %s", *argv);
		return false;
	}

	command_bufaction(range, buffer_delregion.fn);

	return true;
}

bool c_j(int argc, char **argv, bool force, struct range *range)
{
	if(argc != 1){
		ui_err("Usage: %s", *argv);
		return false;
	}

	command_bufaction(range, buffer_joinregion.fn);

	return true;
}

struct g_ctx
{
	bool g_inverse;
	char *match;
	buffer_t *buf;

	const cmd_t *cmd;
	char *str_range;
	int argc;
	char **argv;
	bool force;
};

static bool g_exec(list_t *l, int y, struct g_ctx *ctx)
{
	if(!!tim_strstr(l->line, l->len_line, ctx->match) == ctx->g_inverse)
		return true;

	*ctx->buf->ui_pos = (point_t){ .y = y };

	struct range range, *prange = &range;
	char *end;
	switch(parse_range(ctx->str_range, &end, prange)){
		case RANGE_PARSE_FAIL:
			return true; /* shouldn't hit this, should be caught in g_c() */
		case RANGE_PARSE_NONE:
			prange = NULL;
		case RANGE_PARSE_PASS:
			break;
	}

	cmd_dispatch(ctx->cmd,
			ctx->argc, ctx->argv,
			ctx->force, prange);

	return true;
}

bool c_g(char *cmd, char *gcmd, bool inverse, struct range *range)
{
	const char reg_sep = *gcmd;

	if(reg_sep == '\\'){
		ui_err("can't use '\\' as a separator");
		return false;
	}

	struct
	{
		char *start, *end;
	} regex = { .start = gcmd + 1 };

	for(regex.end = regex.start; *regex.end; regex.end++)
		if(*regex.end == '\\')
			regex.end++;
		else if(*regex.end == reg_sep)
			break;

	char *subcmd = "";
	if(*regex.end == reg_sep){
		*regex.end = '\0';
		subcmd = regex.end + 1;
	}
	/* else, no terminating character */

	buffer_t *const b = buffers_cur();

	struct g_ctx ctx = {
		.g_inverse = inverse,
		.match = regex.start,
		.buf = b,
	};

	struct cmd_t c_p_fallback = {
		.cmd = "p",
		.f_argv = &c_p,
		.single_arg = false,
	};

	/* parse the range here to check, and on each invocation,
	 * since g/a/-1p means current-pos-less-one */
	if(!parse_ranged_cmd(
			subcmd, &ctx.cmd,
			&ctx.argv, &ctx.argc,
			&ctx.force,
			&(struct range *){
				&(struct range){ 0 }
			}))
	{
		if(!*subcmd){
			ctx.argc = 1;
			ctx.argv = umalloc(2 * sizeof *ctx.argv);
			ctx.argv[0] = ustrdup("p");
			ctx.force = false;
			ctx.str_range = NULL;
			ctx.cmd = &c_p_fallback;
		}else{
			ui_err("unknown command: %s", subcmd);
			goto out;
		}
	}else{
		ctx.str_range = subcmd;
	}

	struct range rng_all;
	if(!range){
		rng_all.start = 0;
		rng_all.end = buffer_nlines(b);
		range = &rng_all;
	}

	list_clear_flag(b->head);

	list_flag_range(b->head, range, 1);

	/* need to start at the beginning each time,
	 * in case we've deleted any lines.
	 * Room for optimisation here */
	while(1){
		int y;
		list_t *l = list_flagfind(b->head, 1, &y);
		if(!l)
			break;

		l->flag = 0;
		g_exec(l, y, &ctx);
	}

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
