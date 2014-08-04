#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h> /* offsetof() */
#include <ctype.h>
#include <errno.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "yank.h"
#include "range.h"

#include "io.h"
#include "ui.h"
#include "motion.h"
#include "io.h"
#include "cmds.h"
#include "keys.h"
#include "ncurses.h"
#include "mem.h"
#include "prompt.h"
#include "map.h"
#include "parse_cmd.h"

#include "buffers.h"

#include "retain.h"

#include "config.h"

int keys_filter(
		enum io io_m,
		char *struc, unsigned n_ents,
		unsigned st_off, unsigned st_sz,
		int mode_off)
{
#define STRUCT_STR(i, st, off) *(const char **)(struc + i * st_sz + st_off)
	int ret = -1;

	bool potential[n_ents];

	if(mode_off >= 0){
		enum buf_mode curmode = buffers_cur()->ui_mode;

		for(unsigned i = 0; i < n_ents; i++){
			enum buf_mode mode = *(enum buf_mode *)(
					struc + i * st_sz + mode_off);

			potential[i] = mode & curmode;
		}
	}else{
		memset(potential, true, sizeof potential);
	}

	unsigned ch_idx = 0;
	char *sofar = NULL;
	size_t nsofar = 0;
	for(;; ch_idx++){
		bool raw;
		int ch = io_getch(io_m, &raw);
		if(raw){
			/* raw char, doesn't match any, get out */
			memset(potential, 0, sizeof potential);
		}

		sofar = urealloc(sofar, ++nsofar);
		sofar[nsofar-1] = ch;

		unsigned npotential = 0;
		unsigned last_potential = 0;

		for(unsigned i = 0; i < sizeof potential; i++){
			if(potential[i]){
				const char *const kstr = STRUCT_STR(i, struc, str_off);
				size_t keys_len = strlen(kstr);

				if(ch_idx >= keys_len || kstr[ch_idx] != ch){
					potential[i] = false;
				}else{
					npotential++;
					last_potential = i;
				}
			}
		}

		switch(npotential){
			case 1:
			{
				/* only accept once we have the full string */
				const char *kstr = STRUCT_STR(last_potential, struc, str_off);
				if(ch_idx == strlen(kstr) - 1){
					ret = last_potential;
					goto out;
				}
				break;
			}
			case 0:
				for(size_t i = nsofar; i > 0; i--)
					io_ungetch(sofar[i - 1], false);
				goto out;
		}
	}

out:
	free(sofar);
	return ret;
}

const motion *motion_read(unsigned *repeat, bool apply_maps)
{
	const enum io io_m =
		apply_maps ? bufmode_to_iomap(buffers_cur()->ui_mode) : IO_NOMAP;

	*repeat = io_read_repeat(io_m);

	int i = keys_filter(
			io_m,
			(char *)motion_keys, motion_keys_cnt,
			offsetof(motionkey_t, keys), sizeof(motionkey_t),
			-1);

	if(i == -1)
		return NULL;
	return &motion_keys[i].motion;
}

static
const motion *motion_read_or_visual(unsigned *repeat, bool apply_maps)
{
	buffer_t *buf = buffers_cur();
	if(buf->ui_mode & UI_VISUAL_ANY){
		static motion visual = {
			.func = m_visual,
			.arg.phow = &visual.how
		};

		*repeat = 0;

		return &visual;
	}

	return motion_read(repeat, apply_maps);
}

void k_prompt_cmd(const keyarg_u *arg, unsigned repeat, const int from_ch)
{
	char *const cmd = prompt(from_ch);

	if(!cmd)
		goto cancel_cmd;

	const cmd_t *cmd_f;
	char **argv;
	int argc;
	bool force;
	struct range rstore, *range = &rstore;

	if(parse_ranged_cmd(
			cmd,
			&cmd_f,
			&argv, &argc,
			&force, &range))
	{
		cmd_dispatch(cmd_f, argc, argv, force, range);
	}
	else
	{
		ui_err("unknown command %s", cmd);
	}

	free_argv(argv, argc);
cancel_cmd:
	free(cmd);
}

void k_docmd(const keyarg_u *arg, unsigned repeat, const int from_ch)
{
	char *arg_dup = ustrdup(arg->cmd.arg);
	arg->cmd.fn(1, (char *[]){ arg_dup, NULL }, arg->cmd.force, /*range:*/NULL);
	free(arg_dup);
}

void k_redraw(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	(void)a;
	ui_clear();
	ui_redraw();
	ui_cur_changed();
}

void k_escape(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	const bool was_insert = buf->ui_mode & UI_INSERT_ANY;

	/* set normal mode before the move - lets us capture the position */
	ui_set_bufmode(UI_NORMAL);

	if(was_insert){
		motion move = {
			.func = m_move,
			.arg = { .pos = { -1, 0 } }
		};

		motion_apply_buf(&move, /*repeat:*/1, buf);
	}
}

void k_scroll(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	if(a->pos.x){
		/* y: 0=mid, -1=top, 1=bot */
		int h = 0;
		switch(a->pos.y){
			case 0:
				h = buf->screen_coord.h / 2;
				break;
			case -1:
				break;
			case 1:
				h = buf->screen_coord.h - 1;
				break;
		}
		buf->ui_start.y = buf->ui_pos->y - h;
	}else{
		buf->ui_start.y += a->pos.y;
	}

	if(buf->ui_start.y < 0)
		buf->ui_start.y = 0;

	if(buf->ui_pos->y < buf->ui_start.y)
		buf->ui_pos->y = buf->ui_start.y;
	else if(buf->ui_pos->y >= buf->ui_start.y + buf->screen_coord.h)
		buf->ui_pos->y = buf->ui_start.y + buf->screen_coord.h - 1;

	ui_redraw();
	ui_cur_changed();
}

void k_winsel(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf;
	char dir;

	(void)a;

	buf = buffers_cur();
	bool raw;
	dir = io_getch(IO_NOMAP, &raw);
	(void)raw;

	switch(dir){
#define DIRECT(c, n) case c: buf = buf->neighbours[n]; break

		DIRECT('j', BUF_DOWN);
		DIRECT('k', BUF_UP);
		DIRECT('h', BUF_LEFT);
		DIRECT('l', BUF_RIGHT);

		default:
			return;
	}


	if(buf){
		buffers_set_cur(buf);
		ui_redraw();
		ui_cur_changed();
	}else{
		ui_err("no buffer");
	}
}

void k_show(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();
	(void)a;
	ui_status("%s%s%s, x=%d y=%d eol=%c",
			buf->fname ? "\"" : "",
			buf->fname ? buffer_shortfname(buf->fname) : "<no name>",
			buf->fname ? "\"" : "",
			buf->ui_pos->x, buf->ui_pos->y,
			"ny"[buf->eol]);
}

void k_open(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_insline(buffers_cur(), a->i);
	ui_set_bufmode(UI_INSERT);
	ui_redraw();
	ui_cur_changed();
}

struct replace_ctx
{
	char with;
	unsigned x;
};

static bool replace_iter(char *ch, list_t *l, int y, void *vctx)
{
	const struct replace_ctx *ctx = vctx;

	if(l->len_malloc <= ctx->x){
		l->len_malloc = l->len_line = ctx->x + 2;
		l->line = urealloc(l->line, l->len_malloc);
	}

	l->line[ctx->x] = ctx->with;
	//for(unsigned i = 0; i <= ctx->x; i++)
		//l->line[i] = ctx->with;

	return true;
}

void k_replace(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	if(a->i == 1){
		// TODO: repeated
	}else{
		/* single char */
		bool raw;
		int ch = io_getch(IO_NOMAP, &raw);

		if(!raw && ch == K_ESC)
			return;

		repeat = DEFAULT_REPEAT(repeat);

		motion move_repeat = {
			.func = m_move,
			.arg.pos.x = repeat - 1,
			.how = M_NONE
		};

		buffer_t *buf = buffers_cur();
		const motion *mchosen =
			buf->ui_mode & UI_VISUAL_ANY
			? motion_read_or_visual(&(unsigned){0}, false)
			: &move_repeat;


		region_t r;
		if(!motion_to_region(mchosen, 1, false, buf, &r))
			return;

		/* special case - single _line_ replace
		 * we differ from vim in that multi-char-single-line
		 * replace adds N many '\n's
		 */
		const bool ins_nl = r.type == REGION_CHAR
			&& r.begin.y == r.end.y
			&& (!raw && ch == '\r');

		if(ins_nl)
			ch = '\n';

		/* need to ensure we're called in line-mode, so we
		 * can generate lines */
		if(r.type == REGION_CHAR){
			r.type = REGION_LINE;
			r.end.y++;
		}

		list_iter_region(
				buf->head, &r, LIST_ITER_EVAL_NL,
				/*create:*/true, replace_iter,
				&(struct replace_ctx){
					.with = ch,
					.x = buf->ui_pos->x
				});

		if(ins_nl){
			buf->ui_pos->x = 0;
			buf->ui_pos->y += repeat;
		}

		buf->modified = true;
		buf->ui_mode = UI_NORMAL;
	}
	ui_redraw();
	ui_cur_changed();
}

void k_motion(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	motion_apply_buf(&a->motion.m, a->motion.repeat, buffers_cur());
}

struct around_motion
{
	void (*fn)(buffer_t *, const region_t *,
			point_t *out, struct around_motion *);

	union
	{
		buffer_action_f *forward;
		const struct filter_input *filter;
		enum case_tog case_ty;
	};
};

static bool around_motion(
		unsigned repeat, const int from_ch,
		bool always_linewise,
		struct around_motion *action,
		region_t *used_region)
{
	motion m_doubletap = {
		.func = m_move,
		.how = M_LINEWISE,
	};

	unsigned repeat_motion;
	const motion *m = motion_read_or_visual(
			&repeat_motion, /* operator pending - no maps: */ false);

	if(!m){
		/* check for dd, etc */
		bool raw;
		int ch = io_getch(IO_NOMAP, &raw);
		(void)raw;

		if(ch == from_ch){
			/* dd - stay where we are, +the repeat */
			m_doubletap.arg.pos.y = DEFAULT_REPEAT(repeat) - 1;
			repeat = 0;
			m = &m_doubletap;
		}else{
			if(ch != K_ESC)
				ui_err("no motion '%c'", ch);
			/*io_ungetch(ch);*/
		}
	}

	if(m){
		if(!repeat && !repeat_motion){
			/* leave repeat as zero
			 * e.g. dG will not set any repeats, telling 'G' / m_eof
			 * to go to the default, end of file.
			 */
		}else{
			repeat = DEFAULT_REPEAT(repeat) * DEFAULT_REPEAT(repeat_motion);
		}

		buffer_t *b = buffers_cur();

		region_t r;
		if(!motion_to_region(m, repeat, always_linewise, b, &r))
			return false;

		if(used_region)
			*used_region = r;

		/* reset cursor to beginning, then allow adjustments */
		*b->ui_pos = r.begin;
		if(action)
			action->fn(b, &r, b->ui_pos, action);

		ui_set_bufmode(UI_NORMAL);

		ui_redraw();
		ui_cur_changed();

		return true;
	}

	return false;
}

static void buffer_forward(
		buffer_t *buf, const region_t *region,
		point_t *out, struct around_motion *around)
{
	around->forward(buf, region, out);
}

static bool around_motion_bufaction(
		unsigned repeat, const int from_ch,
		struct buffer_action *buf_act, region_t *out_region)
{
	return around_motion(repeat, from_ch, buf_act->always_linewise,
			&(struct around_motion){
				.fn = buffer_forward,
				.forward = buf_act->fn
			}, out_region);
}

void k_set_mode(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	/* if we're in visual-col and told to insert like that... */
	if(buf->ui_mode == UI_VISUAL_COL && a->i == UI_INSERT_COL){
		region_t r;

		if(around_motion(repeat, from_ch, false, /*noop:*/NULL, &r)){
			buf->col_insert_height = r.end.y - r.begin.y + 1;
			ui_set_bufmode(UI_INSERT_COL);
		}
	}else{
		ui_set_bufmode(a->i);
	}
}

void k_del(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion_bufaction(repeat, from_ch, &buffer_delregion, NULL);
}

void k_change(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	region_t r;

	if(around_motion_bufaction(repeat, from_ch, &buffer_delregion, &r)){
		buffer_t *buf = buffers_cur();

		switch(r.type){
			case REGION_COL:
				buf->col_insert_height = r.end.y - r.begin.y + 1;
				ui_set_bufmode(UI_INSERT_COL);
				break;
			case REGION_LINE:
				k_open(&(keyarg_u){ .i = -1 }, 0, 0);
				break;
			case REGION_CHAR:
				ui_set_bufmode(UI_INSERT);
				break;
		}
	}
}

void k_yank(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion_bufaction(repeat, from_ch, &buffer_yankregion, NULL);
}

void k_join(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion_bufaction(repeat, from_ch, &buffer_joinregion, NULL);
}

void k_indent(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion_bufaction(repeat, from_ch,
			a->i > 0 ? &buffer_indent : &buffer_unindent, NULL);
}

void k_vtoggle(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_togglev(buffers_cur(), a->i != 0);
	ui_cur_changed();
}

void k_go_visual(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	ui_set_bufmode(buf->prev_visual.mode);

	*buf->ui_pos = buf->prev_visual.npos;
	*buffer_uipos_alt(buf) = buf->prev_visual.vpos;

	ui_cur_changed();
}

void k_go_insert(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	*buf->ui_pos = buf->prev_insert;
	ui_set_bufmode(UI_INSERT);

	ui_cur_changed();
}

void k_put(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	bool prepend = a->b;
	const yank *yank = retain(yank_top());
	if(!yank)
		return;

	buffer_t *buf = buffers_cur();
	if(buf->ui_mode & UI_VISUAL_ANY){
		/* delete what we have, then paste */
		region_t r;
		unsigned mrepeat;
		const motion *m = motion_read_or_visual(&mrepeat, false);

		if(!motion_to_region(m, mrepeat, false, buf, &r))
			return;

		/* necessary so we insert at the right place later on */
		point_sort_full(&buf->ui_npos, &buf->ui_vpos);

		buffer_delregion.fn(buf, &r, &(point_t){0});

		/* unconditional for visual-put */
		prepend = true;
	}

	repeat = DEFAULT_REPEAT(repeat);
	while(repeat --> 0)
		buffer_insyank(buffers_cur(), yank, prepend, /*modify:*/true);

	buf->ui_mode = UI_NORMAL; /* remove visual */

	ui_redraw();
	ui_cur_changed();

	release(yank, yank_free);
}

static void filter(
		buffer_t *buf,
		const region_t *region,
		point_t *out,
		struct around_motion *around)
{
	const struct filter_input *pf = around->filter;
	bool free_cmd = false;
	char *cmd;

	switch(pf->type){
		case FILTER_CMD:
			cmd = pf->s;
			if(!cmd){
				cmd = prompt('!');
				if(!cmd)
					return;
				free_cmd = true;
			}
			break;
		case FILTER_SELF:
			cmd = "sh";
	}

	if(buffer_filter(buf, region, cmd))
		ui_err("filter: %s", strerror(errno));

	if(free_cmd)
		free(cmd);
}

void k_filter(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	struct around_motion around = {
		.fn = filter,
		.filter = &a->filter
	};
	region_t r;

	if(around_motion(repeat, from_ch, /*always_linewise:*/true, &around, &r)){
		size_t n = r.end.y - r.begin.y;
		ui_status("filtered %lu line%s", n, n == 1 ? "" : "s");
	}
}

static void case_cb(
		buffer_t *buf,
		const region_t *region,
		point_t *out,
		struct around_motion *around)
{
	buffer_caseregion(buf, around->case_ty, region);
}

void k_case(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	struct around_motion around = {
		.fn = case_cb,
		.case_ty = a->i
	};

	around_motion(repeat, from_ch, /*always_linewise:*/false, &around, NULL);
}

void k_ins_colcopy(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *const buf = buffers_cur();

	list_t *line = buffer_current_line(buf, false);
	if(!line)
		return;
	line = a->i > 0 ? line->next : line->prev;
	if(!line)
		return;

	if((unsigned)buf->ui_pos->x >= line->len_line)
		return;

	buffer_inschar(
			buf,
			line->line[buf->ui_pos->x]);

	ui_redraw();
	ui_cur_changed();
}

void k_normal1(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	const enum buf_mode save = buf->ui_mode;
	buf->ui_mode = UI_NORMAL;

	ui_normal_1(&repeat, IO_MAPRAW | bufmode_to_iomap( buf->ui_mode));

	buf->ui_mode = save;
}
