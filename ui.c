#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "pos.h"
#include "ncurses.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "window.h"
#include "windows.h"
#include "io.h"
#include "ui.h"
#include "motion.h"
#include "io.h"
#include "word.h"
#include "cmds.h"
#include "keys.h"
#include "buffers.h"
#include "mem.h"
#include "str.h"

enum ui_ec ui_run = UI_RUNNING;

static const char *ui_bufmode_str(enum buf_mode m)
{
	switch(m){
		case UI_NORMAL: return "NORMAL";
		case UI_INSERT_COL: return "INSERT BLOCK";
		case UI_INSERT: return "INSERT";
		case UI_VISUAL_CHAR: return "VISUAL";
		case UI_VISUAL_LN: return "VISUAL LINE";
		case UI_VISUAL_COL: return "VISUAL BLOCK";
	}
	return "?";
}

void ui_set_bufmode(enum buf_mode m)
{
	window *win = windows_cur();

	if(window_setmode(win, m)){
		ui_err("bad mode 0x%x", m);
	}else{
		ui_redraw();
		ui_cur_changed();
		nc_style(COL_BROWN);
		ui_status("%s", ui_bufmode_str(win->ui_mode));
		nc_style(0);
	}
}

void ui_init()
{
	nc_init();
}

static
void ui_vstatus(bool err, const char *fmt, va_list l, int right)
{
	int y, x;

	nc_get_yx(&y, &x);

	if(err)
		nc_style(COL_BG_RED);

	char *message = ustrvprintf(fmt, l);
	/* just truncate in the middle if too long */
	const size_t msglen = strlen(message);
	const int cols = nc_COLS();

	if(msglen >= (unsigned)cols){
		const int dotslen = 3;
		int half = (cols - dotslen) / 2;
		char *replace = umalloc(msglen + 1);

		snprintf(replace, msglen + 1, "%.*s...%s",
				half, message, message + msglen - half);

		free(message);
		message = replace;
	}

	nc_status(message, right);
	if(err)
		nc_style(0);

	nc_set_yx(y, x);

	free(message);
}

void ui_status(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	ui_vstatus(false, fmt, l, 0);
	va_end(l);
}

void ui_err(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	ui_vstatus(true, fmt, l, 0);
	va_end(l);
}

static
void ui_rstatus(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	ui_vstatus(false, fmt, l, 1);
	va_end(l);
}

static void ui_match_paren(window *win)
{
	list_t *l = window_current_line(win, false);
	int x = win->ui_pos->x;
	bool redraw = false;

	if(win->ui_paren.x != -1)
		redraw = true;

	win->ui_paren = (point_t){ -1, -1 };

	if(!l || (size_t)x >= l->len_line)
		goto out;

	char ch = l->line[x];
	char other;
	if(!paren_match(ch, &other))
		goto out;

	motion opposite = {
		.func = m_paren,
		.arg.i = other,
		.how = M_EXCLUSIVE
	};
	point_t loc;
	if(!motion_apply_win_dry(&opposite, 0, win, &loc))
		goto out;

	win->ui_paren = loc;
	redraw = true;

out:
	if(redraw)
		ui_redraw();
}

static void ui_handle_next_ch(enum io io_mode, unsigned repeat)
{
	extern nkey_t nkeys[];
	extern const size_t nkeys_cnt;

	int i = keys_filter(
			io_mode, (char *)nkeys, nkeys_cnt,
			offsetof(nkey_t, str), sizeof(nkey_t),
			offsetof(nkey_t, mode));

	if(i != -1){
		nkeys[i].func(&nkeys[i].arg, repeat, nkeys[i].str[0]);
		/* found and handled */
	}else{
		/* not found/handled */
		bool raw;
		int ch = io_getch(0, &raw, true); /* pop it back */
		(void)raw; /* straight out inserted */

		if(ch == 0 || (char)ch == -1) /* eof */
			ui_run = UI_EXIT_1;

		window *const win = windows_cur();

		if(win->ui_mode & UI_INSERT_ANY){
			window_inschar(win, ch);
			ui_redraw(); // TODO: queue these
			ui_cur_changed();
		}else if(ch != K_ESC){
			ui_err("unknown key %c", ch); // TODO: queue?
		}
	}
}

void ui_normal_1(unsigned *repeat, enum io io_mode)
{
	window *win = windows_cur();
	const bool ins = win->ui_mode & UI_INSERT_ANY;

	if(!ins){
		const motion *m = motion_read(repeat, /*map:*/true);
		if(m){
			motion_apply_win(m, *repeat, win);

			/* check if we're now on a paren */
			ui_match_paren(win);

			return;
		} /* else no motion */
	} /* else insert */

	ui_handle_next_ch(io_mode, *repeat);
}

int ui_main()
{
	ui_redraw(); /* this first, to frame buf_sel */
	ui_cur_changed(); /* this, in case there's an initial buf offset */

	while(1){
		ui_wait_return();

		switch(ui_run){
			case UI_RUNNING: break;
			case UI_EXIT_0: return 0;
			case UI_EXIT_1: return 1;
		}

		window *win = windows_cur();

		if(win->ui_mode & UI_VISUAL_ANY){
			point_t *alt = window_uipos_alt(win);

			ui_rstatus("%d,%d - %d,%d",
					win->ui_pos->y, win->ui_pos->x,
					alt->y, alt->x);
		}

		const enum io io_mode = bufmode_to_iomap(win->ui_mode);

		unsigned repeat = 0;
		ui_normal_1(&repeat, io_mode | IO_MAPRAW);
	}
}

void ui_term()
{
	nc_term();
}

void ui_cur_changed()
{
	int need_redraw = 0;
	window *win = windows_cur();
	const int nl = window_nscreenlines(win);

	if(win->ui_pos->x < 0)
		win->ui_pos->x = 0;

	if(win->ui_pos->y < 0)
		win->ui_pos->y = 0;

	if(win->ui_pos->y > win->ui_start.y + nl - 1){
		win->ui_start.y = win->ui_pos->y - nl + 1;
		need_redraw = 1;
	}else if(win->ui_pos->y < win->ui_start.y){
		win->ui_start.y = win->ui_pos->y;
		need_redraw = 1;
	}

	if(win->ui_mode & UI_VISUAL_ANY)
		need_redraw = 1;

	if(need_redraw)
		ui_redraw();

	point_t cursor = window_toscreen(win, win->ui_pos);
	nc_set_yx(cursor.y, cursor.x);
}

static
void ui_draw_hline(int y, int x, int w)
{
	nc_set_yx(y, x);
	while(w --> 0)
		nc_addch('-');
}

static
void ui_draw_vline(int x, int y, int h)
{
	while(h --> 0){
		nc_set_yx(y++, x);
		nc_addch('|');
	}
}

static enum region_type ui_mode_to_region(enum buf_mode m)
{
	switch(m){
		case UI_NORMAL:
		case UI_INSERT:
		case UI_INSERT_COL:
			break;
		case UI_VISUAL_CHAR: return REGION_CHAR;
		case UI_VISUAL_COL: return REGION_COL;
		case UI_VISUAL_LN: return REGION_LINE;
	}
	return REGION_CHAR; /* doesn't matter */
}

static
void ui_draw_win_1(window *win)
{
	buffer_t *buf = win->buf;
	const region_t hlregion = {
		win->ui_npos,
		win->ui_vpos,
		ui_mode_to_region(win->ui_mode),
	};

	list_t *l;
	int y;
	int buf_y;
	for(y = 0, buf_y = win->ui_start.y, l = list_seek(buf->head, buf_y, 0);
			l && y < win->screen_coord.h;
			l = l->next, y++, buf_y++)
	{
		nc_set_yx(win->screen_coord.y + y, win->screen_coord.x);
		nc_clrtoeol();

		unsigned x = 0, i = 0;
		unsigned nwrapped = 0;
		for(; i < l->len_line; i++, x++){
			if(x == (unsigned)win->screen_coord.w - 1 && i + 1 < l->len_line){
				nc_addch('\\');
				x = -1; /* ready for ++ */
				y++;
				nwrapped++;
				if(y == win->screen_coord.h)
					break;
				nc_set_yx(win->screen_coord.y + y, win->screen_coord.x);
			}

			if(win->ui_mode & UI_VISUAL_ANY){
				bool hl = region_contains(&hlregion, &(point_t){ i, buf_y });
				nc_highlight(hl);
			}

			int offhl = 0;

			if(point_eq(&win->ui_paren, (&(point_t){ .x=i, .y=buf_y }))){
				nc_highlight(1);
				offhl = 1;
			}

			nc_addch(l->line[i]);

			if(offhl)
				nc_highlight(0);
		}
		if(nwrapped)
			nc_clrtoeol();

		nc_highlight(0);
	}

	nc_style(COL_BLUE | ATTR_BOLD);
	for(; y < win->screen_coord.h; y++){
		nc_set_yx(win->screen_coord.y + y, win->screen_coord.x);
		nc_addch('~');
		nc_clrtoeol();
	}
	nc_style(0);
}

static
void ui_draw_window_col(window *win)
{
	for(unsigned i = 0; win; i++, win = win->neighbours.below){
		if(i){
			const rect_t *r = &win->screen_coord;
			ui_draw_hline(r->y - 1, r->x, r->w - 1);
		}

		ui_draw_win_1(win);
	}
}

void ui_redraw()
{
	int save_y, save_x;
	nc_get_yx(&save_y, &save_x);

	window *win = window_topleftmost(windows_cur());

	unsigned const screenheight = nc_LINES();
	window_calc_rects(win, nc_COLS(), screenheight);

	for(int i = 0; win; i++, win = win->neighbours.right){
		if(i){
			const rect_t *r = &win->screen_coord;
			/* r->y should be zero, r->h-1 should be screen height */
			ui_draw_vline(r->x - 1, r->y, screenheight - 1);
		}

		ui_draw_window_col(win);
	}

	nc_set_yx(save_y, save_x);
}

void ui_clear(void)
{
	nc_clearall();
}

void ui_printf(const char *fmt, ...)
{
	nc_set_yx(nc_LINES() - 1, 0);

	va_list l;
	va_start(l, fmt);
	nc_vprintf(fmt, l);
	va_end(l);
}

void ui_print(const char *s, size_t n)
{
	nc_set_yx(nc_LINES() - 1, 0);
	while(n --> 0)
		nc_addch(*s++);
	nc_addch('\n');
}

static bool want_return;

void ui_want_return(void)
{
	want_return = true;
}

void ui_wait_return(void)
{
	if(!want_return)
		return;

	want_return = false;

	bool raw;
	int ch = io_getch(IO_NOMAP, &raw, true);
	(void)raw; /* pushed straight back */
	if(!isnewline(ch))
		io_ungetch(ch, false);

	ui_redraw();
	ui_cur_changed();
}
