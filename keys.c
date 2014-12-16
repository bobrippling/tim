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
#include "window.h"
#include "windows.h"
#include "yank.h"
#include "range.h"

#include "io.h"
#include "ui.h"
#include "motion.h"
#include "io.h"
#include "word.h"
#include "cmds.h"
#include "keys.h"
#include "ncurses.h"
#include "mem.h"
#include "prompt.h"
#include "map.h"
#include "str.h"
#include "external.h"
#include "ctags.h"
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
		enum buf_mode curmode = windows_cur()->ui_mode;

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
		int ch = io_getch(io_m, &raw, /*map:*/ch_idx == 0); /* dl shouldn't map l */
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
		apply_maps ? bufmode_to_iomap(windows_cur()->ui_mode) : IO_NOMAP;

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
	window *win = windows_cur();
	if(win->ui_mode & UI_VISUAL_ANY){
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
	char *const cmd = prompt(from_ch, PROMPT_FILENAME);

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
	const char **i;
	char **argv;
	size_t l;

	for(l = 0, i = arg->cmd.argv; i && *i; i++, l++);

	argv = umalloc((l + 1) * sizeof *argv);

	for(l = 0, i = arg->cmd.argv; i && *i; i++, l++)
		argv[l] = ustrdup(*i);

	cmd_dispatch(&arg->cmd.fn, 1, argv, arg->cmd.force, /*range:*/NULL);

	if(arg->cmd.argv[0])
		for(l = 0; argv[l]; l++)
			free(argv[l]);
	free(argv);
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
	window *win = windows_cur();

	const bool was_insert = win->ui_mode & UI_INSERT_ANY;

	/* set normal mode before the move - lets us capture the position */
	ui_set_bufmode(UI_NORMAL);

	if(was_insert){
		motion move = {
			.func = m_move,
			.arg = { .pos = { -1, 0 } }
		};

		motion_apply_win(&move, /*repeat:*/1, win);
	}
}

void k_scroll(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();

	if(a->pos.x){
		/* y: 0=mid, -1=top, 1=bot */
		int h = 0;
		switch(a->pos.y){
			case 0:
				h = win->screen_coord.h / 2;
				break;
			case -1:
				break;
			case 1:
				h = win->screen_coord.h - 1;
				break;
		}
		win->ui_start.y = win->ui_pos->y - h;
	}else{
		win->ui_start.y += a->pos.y;
	}

	if(win->ui_start.y < 0)
		win->ui_start.y = 0;

	if(win->ui_pos->y < win->ui_start.y)
		win->ui_pos->y = win->ui_start.y;
	else if(win->ui_pos->y >= win->ui_start.y + win->screen_coord.h)
		win->ui_pos->y = win->ui_start.y + win->screen_coord.h - 1;

	ui_redraw();
	ui_cur_changed();
}

static enum neighbour pos2dir(const point_t *pos)
{
	/**/ if(pos->x > 0) return neighbour_right;
	else if(pos->x < 0) return neighbour_left;
	else if(pos->y > 0) return neighbour_down;
	else if(pos->y < 0) return neighbour_up;
	else return 0;
}

void k_winsel(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	enum neighbour dir = pos2dir(&a->pos);

	window *const curwin = windows_cur();
	window *found = NULL;

	switch(dir){
		case neighbour_up:
			found = curwin->neighbours.above;
			break;

		case neighbour_down:
			found = curwin->neighbours.below;
			break;

		case neighbour_left:
		case neighbour_right:
			for(found = curwin; found->neighbours.above; found = found->neighbours.above);
			found = (dir == neighbour_left ? found->neighbours.left : found->neighbours.right);
			if(!found)
				break;

			const int to_match = curwin->screen_coord.y + curwin->ui_pos->y - curwin->ui_start.y;

			while(found->neighbours.below)
			{
				if(found->screen_coord.y <= to_match
				&& to_match <= found->screen_coord.y + found->screen_coord.h)
				{
					break;
				}
				found = found->neighbours.below;
			}
	}

	if(found && found != curwin){
		windows_set_cur(found);
		ui_redraw();
		ui_cur_changed();
	}else{
		ui_err("no buffers in that direction");
	}
}

void k_winmove(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	enum neighbour dir = pos2dir(&a->pos);
	int count = 0;

	window *w;
	ITER_WINDOWS(w){
		count++;
		if(count == 2){
			/* now have >1 */
			break;
		}
	}
	if(count < 2)
		return;

	w = windows_cur();
	window *target = window_next(w);

	window_evict(w);

	switch(dir){
		case neighbour_up:
			for(; target->neighbours.above;
					target = target->neighbours.above);
			break;

		case neighbour_down:
			for(; target->neighbours.below;
					target = target->neighbours.below);
			break;

		case neighbour_right:
			for(; target->neighbours.above;
					target = target->neighbours.above);
			for(; target->neighbours.right;
					target = target->neighbours.right);
			break;

		case neighbour_left:
			for(; target->neighbours.above;
					target = target->neighbours.above);
			for(; target->neighbours.left;
					target = target->neighbours.left);
			break;
	}

	window_add_neighbour(target, dir, w);

	ui_redraw();
	ui_cur_changed();
}

void k_show(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();
	buffer_t *buf = win->buf;
	(void)a;
	ui_status("%s%s%s, %sx=%d y=%d eol=%c",
			buf->fname ? "\"" : "",
			buf->fname ? buf->fname : "<no name>",
			buf->fname ? "\"" : "",
			buf->modified ? "[+] " : "",
			win->ui_pos->x, win->ui_pos->y,
			"ny"[buf->eol]);
}

void k_showch(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();
	list_t *l = window_current_line(win, false);
	if(l){
		if((unsigned)win->ui_pos->x < l->len_line){
			const int ch = l->line[win->ui_pos->x];

			ui_status("<%s> %d Hex %x Octal %o",
					ch ? (char[]){ ch, 0 } : "^@",
					ch, ch, ch);
			return;
		}
	}

	ui_status("NUL");
}

void k_open(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();
	buffer_insline(win->buf, a->i, win->ui_pos);
	ui_set_bufmode(UI_INSERT);
	ui_redraw();
	ui_cur_changed();
}

static bool replace_iter(char *ch, list_t *l, int y, void *ctx)
{
	*ch = *(int *)ctx;
	return true;
}

void k_replace(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	if(a->i == 1){
		// TODO: repeated
	}else{
		/* single char */
		bool raw;
		int ch = io_getch(IO_NOMAP, &raw, true);

		if(!raw && ch == K_ESC)
			return;

		repeat = DEFAULT_REPEAT(repeat);

		motion move_repeat = {
			.func = m_move,
			.arg.pos.x = repeat - 1,
			.how = M_NONE
		};

		window *win = windows_cur();
		const motion *mchosen =
			win->ui_mode & UI_VISUAL_ANY
			? motion_read_or_visual(&(unsigned){0}, false)
			: &move_repeat;


		region_t r;
		if(!motion_to_region(mchosen, 1, false, win, &r))
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

		buffer_t *const buf = win->buf;

		list_iter_region(buf->head, &r, LIST_ITER_EVAL_NL, replace_iter, &ch);

		if(ins_nl){
			win->ui_pos->x = 0;
			win->ui_pos->y += repeat;
		}

		buf->modified = true;
		win->ui_mode = UI_NORMAL;
	}
	ui_redraw();
	ui_cur_changed();
}

void k_motion(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	motion_apply_win(&a->motion.m, a->motion.repeat, windows_cur());
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
		int ch = io_getch(IO_NOMAP, &raw, true);
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

		window *win = windows_cur();

		region_t r;
		if(!motion_to_region(m, repeat, always_linewise, win, &r))
			return false;

		if(used_region)
			*used_region = r;

		/* reset cursor to beginning, then allow adjustments */
		*win->ui_pos = r.begin;
		if(action)
			action->fn(win->buf, &r, win->ui_pos, action);

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
	window *win = windows_cur();

	/* if we're in visual-col and told to insert like that... */
	if(win->ui_mode == UI_VISUAL_COL && a->i == UI_INSERT_COL){
		region_t r;

		if(around_motion(repeat, from_ch, false, /*noop:*/NULL, &r)){
			win->buf->col_insert_height = r.end.y - r.begin.y + 1;
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
	window_togglev(windows_cur(), a->i != 0);
	ui_cur_changed();
}

void k_go_visual(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();
	buffer_t *buf = win->buf;

	ui_set_bufmode(buf->prev_visual.mode);

	*win->ui_pos = buf->prev_visual.npos;
	*window_uipos_alt(win) = buf->prev_visual.vpos;

	ui_cur_changed();
}

void k_go_insert(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();
	buffer_t *buf = win->buf;

	*win->ui_pos = buf->prev_insert;
	ui_set_bufmode(UI_INSERT);

	ui_cur_changed();
}

void k_put(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	bool prepend = a->b;
	const yank *yank = retain(yank_top());
	if(!yank)
		return;

	window *win = windows_cur();
	if(win->ui_mode & UI_VISUAL_ANY){
		/* delete what we have, then paste */
		region_t r;
		unsigned mrepeat;
		const motion *m = motion_read_or_visual(&mrepeat, false);

		if(!motion_to_region(m, mrepeat, false, win, &r))
			return;

		/* necessary so we insert at the right place later on */
		point_sort_full(&win->ui_npos, &win->ui_vpos);

		buffer_t *buf = win->buf;

		buffer_delregion.fn(buf, &r, &(point_t){0});

		/* unconditional for visual-put */
		prepend = true;
	}

	repeat = DEFAULT_REPEAT(repeat);
	while(repeat --> 0)
		buffer_insyank(buffers_cur(), yank, win->ui_pos, prepend, /*modify:*/true);

	win->ui_mode = UI_NORMAL; /* remove visual */

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
				cmd = prompt('!', PROMPT_EXEC);
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
	window *win = windows_cur();

	list_t *line = window_current_line(win, false);
	if(!line)
		return;
	line = a->i > 0 ? line->next : line->prev;
	if(!line)
		return;

	if((unsigned)win->ui_pos->x >= line->len_line)
		return;

	window_inschar(win, line->line[win->ui_pos->x]);

	ui_redraw();
	ui_cur_changed();
}

static void k_on_wordfile(const keyarg_u *a, char *fn(const window *), const char *desc)
{
	char *word = fn(windows_cur());

	if(!word){
		ui_err("no %s under cursor", desc);
		return;
	}

	a->word_action.fn(word, a->word_action.flag);

	free(word);
}

void k_on_word(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	k_on_wordfile(a, window_current_word, "word");
}

void k_on_fname(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	k_on_wordfile(a, window_current_fname, "filename");
}

void word_search(const char *word, bool flag)
{
	m_setlastsearch(ustrdup(word), /*forward:*/true);

	motion m_search = {
		.func = m_searchnext,
		.arg = { flag ? -1 : +1 },
		.how = M_EXCLUSIVE
	};

	motion_apply_win(&m_search, /*repeat:*/1, windows_cur());
}

void word_list(const char *word, bool flag)
{
	buffer_t *b = buffers_cur();

	int hit = 0, n = 0;
	for(list_t *i = b->head; i; i = i->next, n++)
		if(tim_strstr(i->line, i->len_line, word))
			ui_printf("%d: %d %s", ++hit, n, i->line);

	ui_want_return();
}

void word_tag(const char *word, bool flag)
{
	struct ctag_result tag;

	if(buffers_cur()->modified){
		ui_err("buffer modified");
		return;
	}

	if(!ctag_search(word, &tag)){
		ui_err("couldn't find tag \"%s\"", word);
		return;
	}

	char *search = tag.line;

	{
		if(strncmp(search, "/^", 2))
			goto out_badtag;
		char *end = strchr(search, '\0');
		if(end - search < 5)
			goto out_badtag;
		if(!strcmp(end - 2, "$/"))
			end[-2] = '\0';
		search += 2;
	}

	const char *current = buffer_fname(buffers_cur());
	if(current && !strcmp(tag.fname, current)){
		/* we're the same file, don't need to replace/do modification check */
	}else if(!edit_common(tag.fname, false)){
		goto out;
	}

	{
		*windows_cur()->ui_pos = (point_t){ 0 };
		word_search(search, false);
	}

	ui_redraw();
	ui_cur_changed();

out:
	ctag_free(&tag);
	return;
out_badtag:
	ui_err("bad tag '%s'", tag.line);
	goto out;
}

void word_man(const char *word, bool flag)
{
	/* if the word is all hexadecimal, try git show */
	bool hex = true;

	for(const char *p = word; *p; p++){
		if(!isxdigit(*p)){
			hex = false;
			break;
		}
	}

	char *cmd = join(" ",
			(char *[]){ hex ? "git show" : "man", (char *)word },
			2);

	shellout(cmd);

	free(cmd);
}

void word_gofile(const char *fname, bool flag)
{
	if(buffers_cur()->modified){
		ui_err("buffer modified");
		return;
	}

	bool success = false;
	if(flag){
		buffer_t *new;
		const char *err;
		buffer_new_fname(&new, fname, &err);

		if(err){
			ui_err("%s: %s", fname, err);
		}else{
			success = true;
		}

		if(success){
			window *wnew = window_new(new);

			window_add_neighbour(windows_cur(), neighbour_up, wnew);
			buffer_release(new);

			windows_set_cur(wnew);
		}

	}else{
		success = edit_common(fname, false);
	}

	if(success){
		ui_redraw();
		ui_cur_changed();
	}
}

void k_normal1(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();

	const enum buf_mode save = win->ui_mode;
	win->ui_mode = UI_NORMAL;

	ui_normal_1(&repeat, IO_MAPRAW | bufmode_to_iomap(win->ui_mode));

	win->ui_mode = save;
}

void k_inc_dec(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	window *win = windows_cur();
	list_t *line = window_current_line(win, false);

	if(!line)
		return;
	if((unsigned)win->ui_pos->x >= line->len_line)
		return;

	size_t pos = win->ui_pos->x;
	for(; pos < line->len_line; pos++)
		if(isdigit(line->line[pos]) || line->line[pos] == '-')
			break;
	if(pos == line->len_line)
		return;

	char *end;
	long long num = strtoll(&line->line[pos], &end, 0);
	if(end == &line->line[pos])
		return;

	repeat = DEFAULT_REPEAT(repeat);

	num += (signed)repeat * a->i;

	char numbuf[64];
	snprintf(numbuf, sizeof numbuf, "%lld", num);
	const size_t numbuflen_new = strlen(numbuf);
	const size_t numbuflen_old = end - &line->line[pos];

	const long change = numbuflen_new - numbuflen_old;

	if(change > 0){
		if(line->len_line + change > line->len_malloc)
			line->line = urealloc(line->line, line->len_malloc += change);

		/* make way */
		memmove(
				&line->line[pos] + numbuflen_new,
				&line->line[pos] + numbuflen_old,
				line->len_line - pos - numbuflen_old);
	}

	memcpy(&line->line[pos], numbuf, numbuflen_new);

	if(change < 0){
		/* fill back */
		memmove(
				&line->line[pos] + numbuflen_new,
				&line->line[pos] + numbuflen_old,
				line->len_line - pos - numbuflen_old);
	}

	line->len_line += change;
	win->buf->modified = true;

	ui_redraw();
}
