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
#include "io.h"
#include "ui.h"
#include "buffers.h"
#include "external.h"
#include "mem.h"
#include "parse_cmd.h"
#include "str.h"

#define RANGE_NO()                       \
	if(range){                             \
		ui_err(":%s doesn't support ranges", \
				*argv);                          \
		return false;                        \
	}

#define RANGE_TODO(cmd)                    \
	if(range){                               \
		ui_err("TODO: range with :%s", cmd);   \
		return false;                          \
	}

#define RANGE_DEFAULT(range, store, y) \
	if(!range){                          \
		store.start = store.end = y;       \
		range = &store;                    \
	}                                    \
	range_sort(range)

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
	RANGE_TODO(*argv);

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

bool c_r(char *argv0, char *rest, bool via_shell, struct range *range)
{
	buffer_t *const b = buffers_cur();

	struct range rng;
	RANGE_DEFAULT(range, rng, b->ui_pos->y);

	*b->ui_pos = (point_t){ .y = range->start };

	int streamerr;
	FILE *stream;
	int (*stream_close)(FILE *);

	if(!via_shell){
		int argc = 0;
		char **argv = NULL;
		parse_cmd(rest, &argc, &argv);

		if(argc != 1)
			goto usage;

		stream = fopen(argv[0], "r");
		streamerr = errno;
		stream_close = &fclose;

		free_argv(argv, argc);
	}else{
		stream = popen(rest, "r");
		streamerr = errno;
		stream_close = &pclose;
	}

	if(!stream){
		ui_err("%s: %s", rest, strerror(streamerr));
		return false;
	}

	list_t *lines = list_new_file(stream, /*eol:*/&(bool){ false });
	(*stream_close)(stream);

	b->head = list_append(b->head, buffer_current_line(b), lines);

	if(lines)
		b->ui_pos->y++;

	ui_redraw();
	ui_cur_changed();

	return true;
usage:
	ui_err("usage: %s filename | %s!cmd...", argv0, argv0);
	return false;
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

static bool c_xdent(
		int argc, char **argv,
		bool force, struct range *range,
		bool indent)
{
	ui_err("TODO");
	return false;
}

bool c_indent(int argc, char **argv, bool force, struct range *range)
{
	return c_xdent(argc, argv, force, range, true);
}

bool c_unindent(int argc, char **argv, bool force, struct range *range)
{
	return c_xdent(argc, argv, force, range, true);
}

bool c_run(char *cmd, char *rest, bool force, struct range *range)
{
	RANGE_TODO(cmd);

	int r = shellout(rest);

	if(r){
		ui_err("command returned %d", r);
		return false;
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
		struct range *range, buffer_action_f *buf_fn, int end_add)
{
	buffer_t *const b = buffers_cur();

	struct range range_store;
	RANGE_DEFAULT(range, range_store, b->ui_pos->y);

	range->end += end_add;

	buf_fn(
			b,
			&(region_t){
				.begin.y = range->start,
				.end.y = range->end + 1,
				.type = REGION_LINE
			},
			&(point_t){});

	*b->ui_pos = (point_t){ .y = range->end - end_add };

	ui_redraw();
	ui_cur_changed();
}

bool c_m(int argc, char **argv, bool force, struct range *range)
{
#define NO_LINE(ln) do{             \
		ui_err("no such line %d", ln);  \
		return false;                   \
	}while(0)

	if(argc != 2){
		ui_err("Usage: %s address", *argv);
		return false;
	}

	int lno;
	char *addr_end;
	if(!parse_range_1(argv[1], &addr_end, &lno) || *addr_end){
		ui_err("bad address: \"%s\"", argv[1]);
		return false;
	}

	buffer_t *b = buffers_cur();

	struct range range_store;
	RANGE_DEFAULT(range, range_store, b->ui_pos->y);

	if(range->start <= lno && lno <= range->end){
		ui_err("can't move lines into themselves");
		return false;
	}

	list_t *landing = list_seek(b->head, lno, false);
	if(!landing)
		NO_LINE(lno);

	/* move lines over *range to lno */
	list_t *start = list_seek(b->head, range->start, false);
	if(!start)
		NO_LINE(range->start);

	list_t *end = list_seek(b->head, range->end, false);
	if(!end)
		NO_LINE(range->end);

	/* break the bottom link */
	list_t *after = end->next;
	end->next = NULL;
	if(after)
		after->prev = NULL;

	/* break the top link and reattach */
	list_t *before = start->prev;
	if(start->prev){
		start->prev = NULL;

		before->next = after;
		if(after)
			after->prev = before;
	}else{
		b->head = after;
	}

	if(lno == -1){
		/* replace head */

		/* move current head to the end tail */
		end->next = b->head;
		b->head->prev = end;

		/* replace */
		b->head = start;

	}else{
		/* replace landing ->next,
		 * linking up end->next to what was there */
		if(landing->next){
			end->next = landing->next;
			landing->next->prev = end;
		}
		landing->next = start;
		start->prev = landing;
	}

	ui_redraw();
	ui_cur_changed();

	return true;
}

bool c_d(int argc, char **argv, bool force, struct range *range)
{
	if(argc != 1){
		ui_err("Usage: %s", *argv);
		return false;
	}

	command_bufaction(range, buffer_delregion.fn, 0);

	return true;
}

bool c_j(int argc, char **argv, bool force, struct range *range)
{
	if(argc != 1){
		ui_err("Usage: %s", *argv);
		return false;
	}

	command_bufaction(range, buffer_joinregion.fn, 1);

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
	if(*ctx->match /* "" match every line */
	&& !!tim_strstr(l->line, l->len_line, ctx->match) == ctx->g_inverse)
	{
		return true;
	}

	*ctx->buf->ui_pos = (point_t){ .y = y };

	struct range range, *prange = &range;
	if(ctx->str_range){
		char *end;
		switch(parse_range(ctx->str_range, &end, prange)){
			case RANGE_PARSE_FAIL:
				return true; /* shouldn't hit this, should be caught in g_c() */
			case RANGE_PARSE_NONE:
				prange = NULL;
			case RANGE_PARSE_PASS:
				break;
		}
	}else{
		prange = NULL;
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
			ui_err("g: unknown command: %s", subcmd);
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
	}else{
		range_sort(range);
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

bool c_norm(char *cmd, char *normcmd, bool force, struct range *range)
{
	enum io mode = IO_MAP; /* normal mode mappings */
	if(force)
		mode = IO_NOMAP;

	buffer_t *b = buffers_cur();

	struct range rng;
	RANGE_DEFAULT(range, rng, b->ui_pos->y);

	size_t normcmdlen = strlen(normcmd);

	for(int n = range->start;
			n <= range->end;
			n++)
	{
		*b->ui_pos = (point_t){ .y = n };

		size_t const io_empty = io_bufsz();

		/* end with an escape */
		io_ungetch(K_ESC, false);

		/* don't io_map() here - may be in insert mode
		 * when we actually execute */
		for(size_t i = normcmdlen; i > 0; i--)
			io_ungetch(normcmd[i - 1], false);

		do{
			/* run the commands while the buffer is on this line */
			ui_normal_1(/*repeat:*/&(unsigned){ 0 }, mode);
		}while(io_bufsz() > io_empty);
	}

	ui_redraw();
	ui_cur_changed();

	return true;
}

bool cmd_dispatch(
		const cmd_t *cmd_f,
		int argc, char **argv,
		bool force, struct range *range)
{
	force ^= cmd_f->inverse;

	return cmd_f->single_arg
		? cmd_f->f_arg1(argv[0], argv[1], force, range)
		: cmd_f->f_argv(argc, argv, force, range);
}
