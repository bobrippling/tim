#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "pos.h"
#include "ui.h"
#include "ncurses.h"
#include "list.h"
#include "buffer.h"
#include "motion.h"
#include "keys.h"
#include "buffers.h"

enum ui_mode ui_mode;
int ui_running = 1;

void ui_init()
{
	ui_mode = UI_NORMAL;

	nc_init();
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
			if(buf->ui_pos.x > 0)
				buffer_delchar(buffers_cur(), &buf->ui_pos.x, &buf->ui_pos.y);
			break;

		default:
			buffer_inschar(buffers_cur(), &buf->ui_pos.x, &buf->ui_pos.y, ch);
			break;
	}

	ui_cur_changed();
	ui_redraw();
}

void ui_main()
{
	extern ikey_t keys[];

	ui_redraw(); /* this first, to frame buf_sel */
	ui_cur_changed(); /* this, in case there's an initial buf offset */

	while(ui_running){
		const int first_ch = nc_getch();
		motion_repeat mr = MOTION_REPEAT();

		int found = 0;

		if(ui_mode == UI_NORMAL){
			int skip = 0;
			int ch = first_ch;
			while(motion_repeat_read(&mr, &ch, skip))
				skip++, motion_apply_buf(&mr, buffers_cur());

			found = skip > 0;
		}

		for(int i = 0; keys[i].ch; i++)
			if(keys[i].mode == ui_mode && keys[i].ch == first_ch){
				keys[i].func(&keys[i].arg, mr.repeat, first_ch);
				found = 1;
			}

		if(!found){
			/* checks for multiple */
			if(ui_mode == UI_INSERT)
				ui_inschar(first_ch);
			else if(mr.repeat)
				; /* ignore */
			else
				ui_status("unknown key %d", first_ch);
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

	if(buf->ui_pos.x < 0)
		buf->ui_pos.x = 0;

	if(buf->ui_pos.y < 0)
		buf->ui_pos.y = 0;

	if(buf->ui_pos.y > buf->ui_start.y + nl - 1){
		buf->ui_start.y = buf->ui_pos.y - nl + 1;
		need_redraw = 1;
	}else if(buf->ui_pos.y < buf->ui_start.y){
		buf->ui_start.y = buf->ui_pos.y;
		need_redraw = 1;
	}

	nc_set_yx(buf->screen_coord.y + buf->ui_pos.y - buf->ui_start.y,
			      buf->screen_coord.x + buf->ui_pos.x - buf->ui_start.x);

	if(need_redraw)
		ui_redraw();
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

static
void ui_draw_buf_1(buffer_t *buf, const rect_t *r)
{
	list_t *l;
	int y;

	buf->screen_coord = *r;

	for(y = 0, l = list_seek(buf->head, buf->ui_start.y, 0); l && y < r->h; l = l->next, y++){
		const int lim = l->len_line < (unsigned)r->w ? l->len_line : (unsigned)r->w;
		int i;

		nc_set_yx(r->y + y, r->x);
		nc_clrtoeol();

		for(i = 0; i < lim; i++)
			nc_addch(l->line[i]);
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
