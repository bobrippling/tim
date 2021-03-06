#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

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
#include "ncurses.h"
#include "window.h"
#include "windows.h"
#include "prompt.h"
#include "main.h"
#include "tab.h"
#include "tabs.h"

#define RANGE_NO()                       \
	if(range){                             \
		ui_err(":%s doesn't support ranges", \
				*argv);                          \
		return false;                        \
	}

#define RANGE_DEFAULT(range, store, win)           \
	if(!range){                                      \
		(store).start = (store).end = win->ui_pos->y;      \
		range = &store;                                \
	}                                                \
	range_sort(range)

#define ARGV_NO()               \
	if(argc != 1){                \
		ui_err("usage: %s", *argv); \
		return false;               \
	}

static
bool c_split(
		const enum neighbour dir,
		bool const withcurrent,
		int argc, char **argv,
		bool force, struct range *range,
		const bool allow_many);

static bool quit_common(
		bool qall,
		int argc, char **argv,
		bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	if(!force){
		bool modified = false;

		if(qall){
			tab *modtab;
			window *modwin = buffers_modified_excluding(NULL, &modtab);

			if(modwin){
				modified = true;

				tabs_set_cur(modtab);
				tab_set_focus(modtab, modwin);
				ui_redraw();
				ui_cur_changed();
			}
		}else{
			modified = buffers_modified_single(buffers_cur());
		}

		if(modified){
			ui_err("buffer modified");
			return false;
		}
	}

	if(qall){
		ui_run = UI_EXIT_0;
	}else{
		window *win = windows_cur();
		window *focus = window_next(win);

		/* check remaining buffers */
		if(!force && !focus && args_next_fname(false)){
			ui_err("more files to edit");
			return false;
		}

		window_evict(win);
		window_free(win);
		windows_set_cur(focus);

		if(!focus){
			/* fallback to another tab? */
			tab *current = tabs_cur();
			tab *tabfocus = tab_next(current);

			if(tabfocus == current){
				/* no more tabs */
				ui_run = UI_EXIT_0;
				tabfocus = NULL;
			}

			tabs_set_cur(tabfocus);
			tab_evict(current);
			tab_free(current);
		}

		switch(ui_run){
			case UI_EXIT_0:
			case UI_EXIT_1:
				break;
			case UI_RUNNING:
				ui_redraw();
				ui_cur_changed();
				break;
		}
	}

	return true;
}

bool c_q(int argc, char **argv, bool force, struct range *range)
{
	return quit_common(false, argc, argv, force, range);
}

bool c_qa(int argc, char **argv, bool force, struct range *range)
{
	return quit_common(true, argc, argv, force, range);
}

bool c_cq(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	/* no buffer checks */
	ui_run = UI_EXIT_1;

	return true;
}

/* if !fname, edit_common() reloads current file */
bool edit_common(const char *fname, bool const force)
{
	window *win = windows_cur();

	const bool reload = !fname;
	if(reload)
		fname = buffer_fname(win->buf);

	if(!fname){ /* shouldn't be the case */
		ui_err("no filename");
		return false;
	}

	/* if it's a reload, we care about whether the buffer is modified, no matter
	 * how many times it's open. otherwise we are loading another file, which is
	 * fine as long as it's not the only instance of the modified-buffer
	 */
	if(!force && (reload ? win->buf->modified : buffers_modified_single(win->buf))){
		ui_err("buffer modified");
		return false;
	}

	bool ret = false;

	const char *err;
	bool loaded = reload
		? window_reload_buffer(win, &err)
		: window_replace_fname(win, fname, &err);

	if(loaded){
		ui_status("%s: loaded", fname);
		buffers_cur()->modified = false;
		ret = true;
	}else{
		ui_err("%s: %s", fname, err);
		if(!reload){
			/* wipe the buffer, e.g. :e /root/hi */
			buffer_t *empty = buffer_new();
			window_replace_buffer(win, empty);
			buffer_release(empty);
		}
	}

	buffer_set_fname(buffers_cur(), fname);

	ui_redraw();
	ui_cur_changed();

	return ret;
}

bool c_n(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	if(buffers_modified_single(buffers_cur()) && !force){
		ui_err("buffer modified");
		return false;
	}

	char *fname = args_next_fname(true);
	if(!fname){
		ui_err("no more files");
		return false;
	}

	return edit_common(fname, force);
}

static bool write_buf(
		buffer_t *buf, bool const force, bool const newfname)
{
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
			ui_err("stat(%s): %s", fname, strerror(errno));
			return false;
		}
	}

	FILE *f = fopen(fname, "w");

	if(!f){
got_err:
		ui_err("%s: %s", fname, strerror(errno));
		if(f)
			fclose(f);
		return false;
	}

	if(buffer_write_file(buf, -1, f, buf->eol) != 0)
		goto got_err; /* file cleanup handled */

	if(fclose(f)){
		f = NULL;
		goto got_err;
	}

	return true;
}

static bool write_range(char *arg, bool force, struct range *range)
{
	arg = skipspace(arg);

	buffer_t *const buf = buffers_cur();
	const char *wfname;

	list_t *seeked = list_seek(buf->head, range->start, false);

	if(!seeked){
		ui_err("out of range");
		return false;
	}

	const int nlines = range->end - range->start + 1;

	if(!*arg){
		/* :a,b write <no arg here> */
		if(!force){
			ui_err("refusing to write partial buffer without force");
			return false;
		}

		wfname = buffer_fname(buf);

	}else{
		if(*arg == '!')
			return shellout_write(arg + 1, seeked, nlines);

		char *fname = arg;

		if(!strncmp(fname, "\\!", 2)){
			/* change \! to ! */
			memmove(fname, fname + 1, strlen(fname));
		}

		wfname = fname;
	}

	/* partial write to file */
	FILE *f = fopen(wfname, "w");
	if(!f){
		ui_err("open %s: %s", wfname, strerror(errno));
		return false;
	}

	int r = list_write_file(seeked, nlines, f, /*eol:*/true);
	int eno = errno;

	if(r){
		ui_err("write %s: %s", wfname, strerror(eno));
		return false;
	}

	return true;
}

bool c_w(char *cmd, char *arg, bool force, struct range *range)
{
	arg = skipspace(arg);

	if(range)
		return write_range(arg, force, range);

	buffer_t *const buf = buffers_cur();

	bool newfname = false;
	if(*arg){
		const char *old = buffer_fname(buf);

		newfname = !old || strcmp(old, arg);

		buffer_set_fname(buf, arg);
	}

	if(!write_buf(buf, force, newfname))
		return false;

	ui_status("written to \"%s\"", buffer_fname(buf));
	return true;
}

bool c_wa(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	bool success = true;
	bool done_write = false;

	for(tab *tab = tabs_first(); tab; tab = tab->next){
		window *win;
		ITER_WINDOWS_FROM(win, tab->win){
			buffer_t *buf = win->buf;

			if(!force && !buf->modified)
				continue;

			bool subforce = false;
			if(!write_buf(buf, subforce, /*newfname:*/false)){
				tabs_set_cur(tab);
				tab_set_focus(tab, win);
				success = false;
			}else{
				done_write = true;
			}
		}
	}

	if(success){
		if(done_write)
			ui_status("saved all%s buffers", force ? "" : " modified");
		else
			ui_status("no modified buffers");

	}else{
		/* leave the error message emitted by write_buf() */
	}

	return success;
}

bool c_x(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	if(argc > 2){
		ui_err("usage: %s [filename]", *argv);
		return false;
	}

	buffer_t *buf = buffers_cur();

	/* if no modifications, don't try to write
	 * this is done in vi and helps ZZ */
	if(argc == 1 && buf->fname && !buf->modified)
		return c_q(argc, argv, false, range);

	return c_w("w", argc > 1 ? argv[1] : "", false, range)
		&& c_q(1, (char *[]){ "" }, false, NULL);
}

bool c_xa(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	for(tab *tab = tabs_first(); tab; tab = tab->next){
		window *win;
		ITER_WINDOWS_FROM(win, tab->win){
			buffer_t *buf = win->buf;

			if(!buf->modified)
				continue;

			if(!write_buf(buf, force, false)){
				tabs_set_cur(tab);
				tab_set_focus(tab, win);
				return false;
			}
		}
	}

	ui_run = UI_EXIT_0;

	return true;
}

bool c_e(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();

	const char *fname;

	if(argc == 1){
		fname = NULL;
		if(!buffer_fname(buffers_cur())){
			ui_err("no filename");
			return false;
		}
	}else if(argc == 2){
		fname = argv[1];
	}else{
		if(buffers_modified_single(buffers_cur())){
			ui_err("buffer modified");
			return false;
		}

		if(!yesno("%d files given - open in vertical split? (y/N) ", argc - 1))
			return false;

		window *current = windows_cur();

		bool ret = c_split(
				neighbour_down,
				/*use current:*/true,
				argc,
				argv,
				/*force*/false,
				/*range*/NULL,
				/*allow_many:*/true);

		/* close current window */
		windows_set_cur(current);
		quit_common(false, 1, (char *[]){ "", NULL }, false, NULL);

		return ret;
	}

	return edit_common(fname, force);
}

bool c_ene(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	if(!force && buffers_modified_single(buffers_cur())){
		ui_err("buffer modified");
		return false;
	}

	buffer_t *new = buffer_new();
	window_replace_buffer(windows_cur(), new);
	buffer_release(new);

	ui_cur_changed();
	ui_redraw();

	return true;
}

bool c_only(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	if(!force && buffers_modified_excluding(buffers_cur(), NULL)){
		ui_err("another buffer has changes");
		return false;
	}

	window *iter;
	size_t count = 0;
	ITER_WINDOWS(iter)
		count++;

	if(count == 1)
		return true;

	window *const anchor = windows_cur();
	windows_set_cur(window_next(anchor));
	window_evict(anchor);

	/* must iterate manually to avoid accessing free'd memory */
	iter = window_topleftmost(windows_cur());
	window *next_right;
	for(window *h = iter; h; h = next_right){
		next_right = h->neighbours.right;

		window *next_below;
		for(iter = h; iter; iter = next_below){
			next_below = iter->neighbours.below;
			window_free(iter);
		}
	}

	windows_set_cur(anchor);

	ui_cur_changed();
	ui_redraw();

	return true;
}

bool c_r(char *argv0, char *rest, bool via_shell, struct range *range)
{
	window *const win = windows_cur();
	buffer_t *const b = win->buf;

	struct range rng;
	RANGE_DEFAULT(range, rng, win);

	*win->ui_pos = (point_t){ .y = range->start };

	list_t *lines = NULL;

	if(via_shell){
		lines = list_new(NULL);
		int err = list_filter(
				&lines,
				&(region_t){ .type = REGION_LINE, },
				rest);

		const int eno = errno;

		ui_redraw();

		if(err){
			ui_err("read process: %s", strerror(eno));
			return false;
		}

	}else{
		int argc = 0;
		char **argv = NULL;
		parse_cmd(rest, &argc, &argv, /*shell glob*/true);

		if(argc != 1){
			ui_err("%s: too many filenames", argv0);
			free_argv(argv, argc);
			return false;
		}

		FILE *stream = fopen(argv[0], "r");

		if(!stream){
			ui_err("%s: %s", rest, strerror(errno));
			return false;
		}

		free_argv(argv, argc);

		lines = list_new_file(stream, /*eol:*/&(bool){ false });
		fclose(stream);
	}

	b->head = list_append(b->head, window_current_line(win, true), lines);

	if(lines)
		win->ui_pos->y++;

	b->modified = true;

	ui_redraw();
	ui_cur_changed();

	return true;
}

static void attach_buffer(
		buffer_t *buf,
		const enum neighbour dir,
		window *const copy_pos_from)
{
	window *w = window_new(buf);
	buffer_release(buf);

	window_add_neighbour(windows_cur(), dir, w);
	windows_set_cur(w);

	if(copy_pos_from){
		*w->ui_pos = *copy_pos_from->ui_pos;

		w->ui_start = copy_pos_from->ui_start;
		if(neighbour_is_vertical(dir)){
			int diff = (w->ui_pos->y - w->ui_start.y) / 2;

			copy_pos_from->ui_start.y += diff;
			w->ui_start.y += diff;
		}
	}
}

static
bool c_split(
		const enum neighbour dir,
		bool const withcurrent,
		int argc, char **argv,
		bool force, struct range *range,
		const bool allow_many)
{
	RANGE_NO();

	if(!allow_many && argc > 2){
		ui_err("usage: %s [filename]", *argv);
		return false;
	}

	buffer_t *b;

	if(argc > 1){
		for(int i = 1; i < argc; i++){
			const char *err;
			buffer_new_fname(&b, argv[i], &err);

			if(err){
				ui_err("%s: %s", argv[i], err);

				/* edit new file */
				b = buffer_new();
				buffer_set_fname(b, argv[i]);
			}

			attach_buffer(b, dir, NULL);
		}

	}else if(withcurrent){
		window *cur = windows_cur();
		b = retain(cur->buf);
		attach_buffer(b, dir, /*copy pos from:*/cur);

	}else{
		b = buffer_new();
		attach_buffer(b, dir, NULL);
	}

	ui_redraw();
	ui_cur_changed();

	return true;
}

static bool split_multi(int argc)
{
	if(argc <= 2)
		return false; /* doesn't matter - only one anyway */

	return yesno("%d files given - open in split? (y/N) ", argc - 1);
}

bool c_vs(int argc, char **argv, bool force, struct range *range)
{
	return c_split(neighbour_left, true, argc, argv, force, range, split_multi(argc));
}

bool c_vnew(int argc, char **argv, bool force, struct range *range)
{
	return c_split(neighbour_left, false, argc, argv, force, range, split_multi(argc));
}

bool c_sp(int argc, char **argv, bool force, struct range *range)
{
	return c_split(neighbour_up, true, argc, argv, force, range, split_multi(argc));
}

bool c_new(int argc, char **argv, bool force, struct range *range)
{
	return c_split(neighbour_up, false, argc, argv, force, range, split_multi(argc));
}

bool c_all(int argc, char **argv, bool force, struct range *range)
{
	ARGV_NO();
	if(force){
		ui_err("usage: %s", *argv);
		return false;
	}

	if(!args_next_fname(false))
		ui_status("no remaining buffers");

	window *cur = windows_cur();
	for(;;){
		const char *fname = args_next_fname(true);
		if(!fname)
			break;

		buffer_t *buf;
		const char *err;
		buffer_new_fname(&buf, fname, &err);

		window *w = window_new(buf);
		buffer_release(buf);

		window_add_neighbour(cur, neighbour_down, w);

		cur = w;
	}

	ui_redraw();

	return true;
}

bool c_tabe(int argc, char **argv, bool force, struct range *range)
{
	if(argc > 2){
		ui_err("usage: %s [path]", *argv);
		return false;
	}

	buffer_t *buf = buffer_new();

	window *win = window_new(buf);
	tab *tab = tab_new(win);
	buffer_release(buf);

	tab->next = tabs_cur()->next;
	tabs_cur()->next = tab;

	tabs_set_cur(tab);

	if(argc == 1){
		/* empty tab - fine */
	}else if(argc == 2){
		return edit_common(argv[1], force);
	}

	ui_redraw();
	ui_cur_changed();

	return true;
}

bool c_tabo(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	tab *const tab_current = tabs_cur();

	if(!force){
		for(tab *tab = tabs_first(); tab; tab = tab->next){
			if(tab == tab_current)
				continue;

			window *win;
			ITER_WINDOWS_FROM(win, tab->win){
				buffer_t *buf = win->buf;

				/* if we contain this buffer, then it's fine */
				if(tab_contains_buffer(tab_current, buf))
					continue;

				if(buf->modified){
					ui_err("other tab contains modified buffer");
					return false;
				}
			}
		}
	}

	tab_evict(tab_current);

	tab *t = tabs_first();
	while(t){
		tab *next = t->next;

		assert(t != tab_current);
		tab_free(t);

		t = next;
	}

	tabs_set_cur(tab_current);
	tabs_set_first(tab_current);

	ui_redraw();
	ui_cur_changed();

	return false;
}

bool c_run(char *cmd, char *rest, bool force, struct range *range)
{
	if(range){
		/* filter */
		buffer_t *buf = buffers_cur();

		int err = list_filter(
				&buf->head,
				&(region_t){
					.type = REGION_LINE,
					.begin.y = range->start,
					.end.y = range->end,
				},
				rest);

		const int eno = errno;

		ui_redraw();

		if(err){
			ui_err("filter: %s", strerror(eno));
			return false;
		}

		return true;
	}

	int r = shellout(rest);

	if(r){
		ui_err("command returned %d", r);
		return false;
	}

	return true;
}

bool c_ls(int argc, char **argv, bool force, struct range *range)
{
	RANGE_NO();
	ARGV_NO();

	buffer_t *b = buffer_new();
	attach_buffer(b, neighbour_down, NULL);

	list_t *files = NULL, *latest = NULL;

	for(tab *tab = tabs_first(); tab; tab = tab->next){
		window *win;
		ITER_WINDOWS_FROM(win, tab->win){
			buffer_t *buf = win->buf;
			const char *fname = buffer_fname(buf);
			if(fname){
				if(list_contains(files, fname))
					continue;

				list_t *new = list_new(NULL);
				new->line = ustrdup(fname);
				new->len_line = strlen(new->line);
				new->len_malloc = new->len_line + 1;

				files = list_append(files, latest, new);
				latest = new;
			}
		}
	}

	buffer_replace_list(b, files);

	ui_redraw();
	ui_cur_changed();

	return true;
}

bool c_p(int argc, char **argv, bool force, struct range *range)
{
	ARGV_NO();

	window *win = windows_cur();

	struct range rng;
	RANGE_DEFAULT(range, rng, win);

	buffer_t *const b = win->buf;

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
	window *win = windows_cur();

	struct range range_store;
	RANGE_DEFAULT(range, range_store, win);

	range->end += end_add;

	buffer_t *const b = win->buf;

	buf_fn(
			b,
			&(region_t){
				.begin.y = range->start,
				.end.y = range->end,
				.type = REGION_LINE
			},
			&(point_t){});

	*win->ui_pos = (point_t){ .y = range->end - end_add };

	ui_redraw();
	ui_cur_changed();
}

static bool copy_move_common(
		int argc, char **argv,
		struct range **const range,
		struct range *const range_store,
		int *const out_addr)
{
	if(argc != 2){
		ui_err("Usage: %s address", *argv);
		return false;
	}

	char *addr_end;
	if(!parse_range_1(argv[1], &addr_end, out_addr) || *addr_end){
		ui_err("bad address: \"%s\"", argv[1]);
		return false;
	}

	window *win = windows_cur();

	RANGE_DEFAULT(*range, *range_store, win);

	return true;
}

bool c_m(int argc, char **argv, bool force, struct range *range)
{
#define NO_LINE(ln) do{             \
		ui_err("no such line %d", ln);  \
		return false;                   \
	}while(0)

	struct range range_store;
	int lno;
	if(!copy_move_common(argc, argv, &range, &range_store, &lno))
		return false;

	if(range->start <= lno && lno <= range->end){
		ui_err("can't move lines into themselves");
		return false;
	}

	buffer_t *const b = windows_cur()->buf;

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

	b->modified = true;

	ui_redraw();
	ui_cur_changed();

	return true;
}

bool c_t(int argc, char **argv, bool force, struct range *range)
{
	if(argc != 2){
		ui_err("Usage: %s address", *argv);
		return false;
	}

	struct range range_store;
	int lno;
	if(!copy_move_common(argc, argv, &range, &range_store, &lno))
		return false;

	buffer_t *const b = windows_cur()->buf;

	list_t *landing = list_seek(b->head, lno, false);
	if(!landing)
		NO_LINE(lno);

	/* copy the range */
	list_t *start = list_seek(b->head, range->start, false);
	if(!start)
		NO_LINE(range->start);

	list_t *end = list_seek(b->head, range->end, false);
	if(!end)
		NO_LINE(range->end);

	list_t *copied;
	/* break the bottom link temporarily for the copy */
	{
		list_t *after = end->next;
		end->next = NULL;

		copied = list_copy_deep(start, NULL);
		end->next = after;
	}

	if(lno == -1){
		/* special case - head */
		list_t *head = b->head;

		b->head = copied;
		list_t *i;
		for(i = b->head; i->next; i = i->next);

		i->next = head;
		if(head)
			head->prev = i;

	}else{
		list_t *next = landing->next;
		landing->next = copied;
		copied->prev = landing;

		list_t *i;
		for(i = landing; i->next; i = i->next);
		i->next = next;
		if(next)
			next->prev = i;
	}

	b->modified = true;

	ui_redraw();
	ui_cur_changed();

	return true;
}

bool c_d(int argc, char **argv, bool force, struct range *range)
{
	ARGV_NO();

	command_bufaction(range, buffer_delregion.fn, 0);

	return true;
}

bool c_j(int argc, char **argv, bool force, struct range *range)
{
	ARGV_NO();

	if(range && range->start != range->end){
		/* :2j means :2,3j
		 * otherwise, we have :5,6j, in which case we want to say :5,5j */
		range->end--;
	}

	command_bufaction(range, buffer_joinregion_space.fn, 1);

	return true;
}

struct g_ctx
{
	bool g_inverse;
	char *match;
	window *win;

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

	*ctx->win->ui_pos = (point_t){ .y = y };

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

	window *const win = windows_cur();

	struct g_ctx ctx = {
		.g_inverse = inverse,
		.match = regex.start,
		.win = win,
	};

	struct cmd_t c_p_fallback = {
		.arg0 = "p",
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

	buffer_t *const b = win->buf;

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

	window *win = windows_cur();

	struct range rng;
	RANGE_DEFAULT(range, rng, win);

	size_t normcmdlen = strlen(normcmd);

	for(int n = range->start;
			n <= range->end;
			n++)
	{
		*win->ui_pos = (point_t){ .y = n };

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
