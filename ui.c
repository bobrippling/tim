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
		nc_style(COL_BG_RED);
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

static void ui_calc_bufrects(buffer_t *buf,
		const unsigned screenwidth, const unsigned screenheight)
{
	unsigned nbuffers_h = 0;
	buffer_t *buf_h;
	for(buf_h = buf; buf_h; buf_h = buf_h->neighbours.right, nbuffers_h++);

	const unsigned nvlines = nbuffers_h + 1;
	const unsigned bufspace_v = screenwidth - nvlines;
	unsigned bufwidth = bufspace_v / nbuffers_h;

	unsigned i_h;
	for(i_h = 0, buf_h = buf; buf_h; buf_h = buf_h->neighbours.right, i_h++){
		unsigned nbuffers_v = 0;
		buffer_t *buf_v;
		for(buf_v = buf_h; buf_v; buf_v = buf_v->neighbours.below)
			nbuffers_v++;

		/* final buffer gets any left over cols */
		if(!buf_h->neighbours.right)
			bufwidth += bufspace_v % nbuffers_h;

		const unsigned nhlines = nbuffers_v - 1;
		const unsigned bufspace_h = screenheight - /*status*/1 - nhlines;
		const unsigned bufheight = bufspace_h / nbuffers_v;

		unsigned i_v;
		for(i_v = 0, buf_v = buf_h; buf_v; buf_v = buf_v->neighbours.below, i_v++){
			rect_t r = {
				.x = i_h * (bufwidth + 1),
				.y = i_v * (bufheight + 1),
				.w = bufwidth,
				.h = bufheight,
			};

			/* final buffer gets any left over lines */
			if(!buf_v->neighbours.below)
				r.h += bufspace_h % nbuffers_v;

			buf_v->screen_coord = r;
		}
	}
}

static
void ui_draw_buf_1(buffer_t *buf)
{
	const region_t hlregion = {
		buf->ui_npos,
		buf->ui_vpos,
		ui_mode_to_region(buf->ui_mode),
	};

	list_t *l;
	int y;
	for(y = 0, l = list_seek(buf->head, buf->ui_start.y, 0);
			l && y < buf->screen_coord.h;
			l = l->next, y++)
	{
		const int lim = l->len_line < (unsigned)buf->screen_coord.w
			? l->len_line
			: (unsigned)buf->screen_coord.w;

		nc_set_yx(buf->screen_coord.y + y, buf->screen_coord.x);
		nc_clrtoeol();

		for(int x = 0; x < lim; x++){
			if(buf->ui_mode & UI_VISUAL_ANY)
				nc_highlight(region_contains(
							&hlregion, x, buf->ui_start.y + y));

			nc_addch(l->line[x]);
		}

		nc_highlight(0);
	}

	nc_style(COL_BLUE | ATTR_BOLD);
	for(; y < buf->screen_coord.h; y++){
		nc_set_yx(buf->screen_coord.y + y, buf->screen_coord.x);
		nc_addch('~');
		nc_clrtoeol();
	}
	nc_style(0);
}

static
void ui_draw_buf_col(buffer_t *buf)
{
	for(unsigned i = 0; buf; i++, buf = buf->neighbours.below){
		if(i){
			const rect_t *r = &buf->screen_coord;
			ui_draw_hline(r->y - 1, r->x, r->w - 1);
		}

		ui_draw_buf_1(buf);
	}
}

void ui_redraw()
{
	int save_y, save_x;
	nc_get_yx(&save_y, &save_x);

	buffer_t *buf = buffer_topleftmost(buffers_cur());

	unsigned const screenheight = nc_LINES();
	ui_calc_bufrects(buf, nc_COLS(), screenheight);

	for(int i = 0; buf; i++, buf = buf->neighbours.right){
		if(i){
			const rect_t *r = &buf->screen_coord;
			/* r->y should be zero, r->h-1 should be screen height */
			ui_draw_vline(r->x - 1, r->y, screenheight - 1);
		}

		ui_draw_buf_col(buf);
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
