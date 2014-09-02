#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "pos.h"
#include "ncurses.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "io.h"
#include "ui.h"
#include "motion.h"
#include "cmds.h"
#include "io.h"
#include "surround.h"
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
	buffer_t *buf = buffers_cur();

	if(buffer_setmode(buf, m)){
		ui_err("bad mode 0x%x", m);
	}else{
		ui_redraw();
		ui_cur_changed();
		nc_style(COL_BROWN);
		ui_status("%s", ui_bufmode_str(buf->ui_mode));
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

static void ui_match_paren(buffer_t *buf)
{
	list_t *l = buffer_current_line(buf, false);
	const int x = buf->ui_pos->x;
	bool redraw = false;

	if(buf->ui_paren.x != -1)
		redraw = true;

	buf->ui_paren = (point_t){ -1, -1 };

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
	if(!motion_apply_buf_dry(&opposite, 0, buf, buf->ui_pos, &loc))
		goto out;

	buf->ui_paren = loc;
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

		buffer_t *const buf = buffers_cur();

		if(buf->ui_mode & UI_INSERT_ANY){
			buffer_inschar(buf, ch);
			ui_redraw(); // TODO: queue these
			ui_cur_changed();
		}else if(ch != K_ESC){
			ui_err("unknown key %c", ch); // TODO: queue?
		}
	}
}

static bool try_visual_surround(buffer_t *buf, unsigned repeat)
{
	if(buf->ui_mode & UI_VISUAL_ANY){
		/* check surround */
		bool raw;
		int sur_ch = io_getch(IO_MAPV, &raw, /*domaps*/false);

		if(surround_beginning_char(sur_ch)){
			int sur_type = io_getch(IO_NOMAP, &raw, false);
			const surround_key_t *sur = surround_read(sur_type);

			if(sur){
				region_t r;

				if(surround_apply(sur, sur_ch, sur_type, repeat, buf, &r)){
					buf->ui_npos = r.begin;
					buf->ui_vpos = r.end;

					switch(r.type){
						case REGION_CHAR: ui_set_bufmode(UI_VISUAL_CHAR); break;
						case REGION_COL:  ui_set_bufmode(UI_VISUAL_COL); break;
						case REGION_LINE: ui_set_bufmode(UI_VISUAL_LN); break;
					}
				}else{
					ui_status("surround %c%c failed", sur_ch, sur_type);
				}
				return true;
			}
			io_ungetch(sur_type, false);
		}

		io_ungetch(sur_ch, false);
	}
	return false;
}

void ui_normal_1(unsigned *repeat, enum io io_mode)
{
	buffer_t *buf = buffers_cur();
	const bool ins = buf->ui_mode & UI_INSERT_ANY;

	if(!ins){
		const motion *m = motion_read(repeat, /*map:*/true);
		if(m){
			motion_apply_buf(m, *repeat, buf);

			/* check if we're now on a paren */
			ui_match_paren(buf);

			return;
		} /* else no motion */

		if(try_visual_surround(buf, *repeat))
			return;
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

		buffer_t *buf = buffers_cur();

		if(buf->ui_mode & UI_VISUAL_ANY){
			point_t *alt = buffer_uipos_alt(buf);

			ui_rstatus("%d,%d - %d,%d",
					buf->ui_pos->y, buf->ui_pos->x,
					alt->y, alt->x);
		}

		const enum io io_mode = bufmode_to_iomap(buf->ui_mode);

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
	buffer_t *buf = buffers_cur();
	const int nl = buf->screen_coord.h;

	if(buf->ui_pos->x < 0)
		buf->ui_pos->x = 0;

	if(buf->ui_pos->y < 0)
		buf->ui_pos->y = 0;

	if(buf->ui_pos->y > buf->ui_start.y + nl - 1){
		buf->ui_start.y = buf->ui_pos->y - nl + 1;
		need_redraw = 1;
	}else if(buf->ui_pos->y < buf->ui_start.y){
		buf->ui_start.y = buf->ui_pos->y;
		need_redraw = 1;
	}

	if(buf->ui_mode & UI_VISUAL_ANY)
		need_redraw = 1;

	if(need_redraw)
		ui_redraw();

	point_t cursor = buffer_toscreen(buf, buf->ui_pos);
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
void ui_draw_buf_1(buffer_t *buf, const rect_t *r)
{
	buf->screen_coord = *r;

	const region_t hlregion = {
		buf->ui_npos,
		buf->ui_vpos,
		ui_mode_to_region(buf->ui_mode),
	};

	list_t *l;
	int y;
	for(y = 0, l = list_seek(buf->head, buf->ui_start.y, 0);
			l && y < r->h;
			l = l->next, y++)
	{
		const int lim = l->len_line < (unsigned)r->w
			? l->len_line
			: (unsigned)r->w;

		nc_set_yx(r->y + y, r->x);
		nc_clrtoeol();

		for(int x = 0; x < lim; x++){
			int offhl = 0;
			const int real_y = buf->ui_start.y + y;

			if(buf->ui_mode & UI_VISUAL_ANY)
				nc_highlight(region_contains(
							&hlregion, x, real_y));

			if(point_eq(&buf->ui_paren, (&(point_t){ .x=x, .y=real_y }))){
				nc_highlight(1);
				offhl = 1;
			}

			nc_addch(l->line[x]);

			if(offhl)
				nc_highlight(0);
		}

		nc_highlight(0);
	}

	nc_style(COL_BLUE | ATTR_BOLD);
	for(; y < r->h; y++){
		nc_set_yx(r->y + y, r->x);
		nc_addch('~');
		nc_clrtoeol();
	}
	nc_style(0);
}

static
void ui_draw_buf_col(buffer_t *buf, const rect_t *r_col)
{
	buffer_t *bi;
	int i;
	int h;
	int nrows;

	for(nrows = 1, bi = buf;
			bi->neighbours[BUF_DOWN];
			bi = bi->neighbours[BUF_DOWN], nrows++);

	h = r_col->h / nrows;

	for(i = 0; buf; i++, buf = buf->neighbours[BUF_DOWN]){
		rect_t r = *r_col;
		r.y = i * h;
		r.h = h;

		if(i){
			ui_draw_hline(r.y, r.x, r.w - 1);
			r.y++;
			if(buf->neighbours[BUF_DOWN])
				r.h--;
		}

		ui_draw_buf_1(buf, &r);
	}
}

void ui_redraw()
{
	buffer_t *buf, *bi;
	int save_y, save_x;
	int w;
	int ncols, i;

	nc_get_yx(&save_y, &save_x);

	buf = buffer_topleftmost(buffers_cur());

	for(ncols = 1, bi = buf;
			bi->neighbours[BUF_RIGHT];
			bi = bi->neighbours[BUF_RIGHT], ncols++);

	w = nc_COLS() / ncols - (ncols + 1);

	for(i = 0; buf; i++, buf = buf->neighbours[BUF_RIGHT]){
		rect_t r;

		r.x = i * w;
		r.y = 0;
		r.w = w;
		r.h = nc_LINES() - 1;

		if(i){
			ui_draw_vline(r.x, r.y, r.h - 1);
			r.x++;
			if(buf->neighbours[BUF_RIGHT])
				r.w--;
		}

		ui_draw_buf_col(buf, &r);
	}while(buf);

	nc_set_yx(save_y, save_x);
}

void ui_clear(void)
{
	nc_clearall();
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
