#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "pos.h"
#include "ncurses.h"
#include "list.h"
#include "buffer.h"
#include "ui.h"
#include "motion.h"
#include "keys.h"
#include "buffers.h"
#include "io.h"
#include "region.h"

#define UI_MODE() buffers_cur()->ui_mode

int ui_running = 1;

static const char *ui_bufmode_str(enum buf_mode m)
{
	switch(m){
		case UI_NORMAL: return "NORMAL";
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
		ui_status("bad mode 0x%x", m);
	}else{
		ui_redraw();
		ui_cur_changed();
		ui_status("%s", ui_bufmode_str(buf->ui_mode));
	}
}

void ui_init()
{
	nc_init();
	ui_set_bufmode(UI_NORMAL);
}

void ui_status(const char *fmt, ...)
{
	va_list l;
	int y, x;

	nc_get_yx(&y, &x);

	va_start(l, fmt);
	nc_vstatus(fmt, l);
	va_end(l);

	nc_set_yx(y, x);
}

static
void ui_inschar(char ch)
{
	buffer_t *buf = buffers_cur();

	switch(ch){
		case CTRL_AND('?'):
		case CTRL_AND('H'):
		case 127:
			if(buf->ui_pos->x > 0)
				buffer_delchar(buffers_cur(), &buf->ui_pos->x, &buf->ui_pos->y);
			break;

		default:
			buffer_inschar(buffers_cur(), &buf->ui_pos->x, &buf->ui_pos->y, ch);
			break;
	}

	ui_cur_changed();
	ui_redraw();
}

void ui_main()
{
	extern nkey_t nkeys[];

	ui_redraw(); /* this first, to frame buf_sel */
	ui_cur_changed(); /* this, in case there's an initial buf offset */

	while(ui_running){
		if(UI_MODE() != UI_INSERT){
			unsigned repeat = 0;
			const motion *m = motion_read(&repeat);

			if(m){
				motion_apply_buf(m, repeat, buffers_cur());
				continue;
			}
		}

		const bool ins = UI_MODE() == UI_INSERT;
		const enum io io_mode = ins ? IO_NOMAP : IO_MAP;

		unsigned repeat = ins ? 0 : io_read_repeat(io_mode);
		int ch = io_getch(io_mode);

		bool found = false;
		for(int i = 0; nkeys[i].ch; i++)
			if(nkeys[i].mode & UI_MODE() && nkeys[i].ch == ch){
				nkeys[i].func(&nkeys[i].arg, repeat, ch);
				found = true;
			}

		if(!found){
			/* checks for multiple */
			if(UI_MODE() == UI_INSERT)
				ui_inschar(ch);
			else if(ch != K_ESC)
				ui_status("unknown key %c", ch);
		}
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
			if(buf->ui_mode & UI_VISUAL_ANY)
				nc_highlight(region_contains(
							&hlregion, x, buf->ui_start.y + y));

			nc_addch(l->line[x]);
		}

		nc_highlight(0);
	}

	for(; y < r->h; y++){
		nc_set_yx(r->y + y, r->x);
		nc_addch('~');
		nc_clrtoeol();
	}
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
