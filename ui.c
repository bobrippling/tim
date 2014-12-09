#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "pos.h"
#include "ncurses.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "io.h"
#include "ui.h"
#include "motion.h"
#include "cmds.h"
#include "keys.h"
#include "buffers.h"

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
		nc_style(COL_RED);
	nc_vstatus(fmt, l, right);
	if(err)
		nc_style(0);

	nc_set_yx(y, x);
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
		int ch = io_getch(0, &raw); /* pop it back */
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

void ui_normal_1(unsigned *repeat, enum io io_mode)
{
	buffer_t *buf = buffers_cur();
	const bool ins = buf->ui_mode & UI_INSERT_ANY;

	if(!ins){
		const motion *m = motion_read(repeat, /*map:*/true);
		if(m){
			motion_apply_buf(m, *repeat, buf);
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
	const int nl = buffer_nscreenlines(buf);

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
	int buf_y;
	for(y = 0, buf_y = buf->ui_start.y, l = list_seek(buf->head, buf_y, 0);
			l && y < r->h;
			l = l->next, y++, buf_y++)
	{
		nc_set_yx(r->y + y, r->x);
		nc_clrtoeol();

		const unsigned xlim = r->w;
		unsigned x = 0, i = 0;
		unsigned nwrapped = 0;
		for(; i < l->len_line; i++, x++){
			if(buf->ui_mode & UI_VISUAL_ANY){
				bool hl = region_contains(&hlregion, &(point_t){ i, buf_y });
				nc_highlight(hl);
			}

			nc_addch(l->line[i]);

			if(x == xlim && i + 1 < l->len_line){
				nc_addch('\\');
				x = -1; /* ready for ++ */
				y++;
				nwrapped++;
				if(y == r->h)
					break;
				nc_set_yx(r->y + y, r->x);
			}
		}
		if(nwrapped)
			nc_clrtoeol();

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
	int ch = io_getch(IO_NOMAP, &raw);
	(void)raw; /* pushed straight back */
	if(!isnewline(ch))
		io_ungetch(ch, false);

	ui_redraw();
	ui_cur_changed();
}
