#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "pos.h"
#include "ncurses.h"
#include "list.h"
#include "buffer.h"
#include "ui.h"
#include "motion.h"
#include "keys.h"
#include "buffers.h"
#include "io.h"

#define UI_MODE() buffers_cur()->ui_mode

int ui_running = 1;

static const char *ui_bufmode_str(enum buf_mode m)
{
	switch(m){
		case UI_NORMAL: return "NORMAL";
		case UI_INSERT: return "INSERT";
		case UI_VISUAL_LN: return "VISUAL";
	}
	return "?";
}

void ui_set_bufmode(enum buf_mode m)
{
	/* only a single bit */
	if(!m || (m & (m - 1))){
		ui_status("bad mode 0x%x", m);
	}else{
		buffer_t *buf = buffers_cur();

		buf->ui_mode = m;

		if(m == UI_VISUAL_LN)
			buf->ui_vpos = buf->ui_npos;

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
		/* don't want maps if in insert mode */
		const int first_ch = io_getch(
				UI_MODE() == UI_INSERT ? IO_NOMAP : IO_MAP);
		int last_ch = first_ch;

		motion_repeat mr = MOTION_REPEAT();

		int found = 0;

		if(UI_MODE() != UI_INSERT){
			int skip = 0;
			while(motion_repeat_read(&mr, &last_ch, skip))
				skip++, motion_apply_buf(&mr, buffers_cur());

			found = skip > 0;
		}

		for(int i = 0; nkeys[i].ch; i++)
			if(nkeys[i].mode & UI_MODE() && nkeys[i].ch == last_ch){
				nkeys[i].func(&nkeys[i].arg, mr.repeat, last_ch);
				found = 1;
			}

		if(!found){
			/* checks for multiple */
			if(UI_MODE() == UI_INSERT)
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

static void ui_fill_visual(
		point_t *nc_a, point_t *nc_b)
{
	/* put A before B */
	point_minmax(nc_a, nc_b);

	for(int y = nc_a->y; y <= nc_b->y; y++)
		for(int x = nc_a->x; x <= nc_b->x; x++)
			nc_highlight(y, x, 1);
}

static void ui_draw_visual(buffer_t *buf)
{
	switch(buf->ui_mode){
		case UI_NORMAL:
		case UI_INSERT:
			break;
		case UI_VISUAL_LN:
		{
			point_t ncursor = buffer_toscreen(buf, &buf->ui_npos);
			point_t vcursor = buffer_toscreen(buf, &buf->ui_vpos);
			ui_fill_visual(&ncursor, &vcursor);
			break;
		}
	}
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

	if(need_redraw)
		ui_redraw();

	/* must do visual in ui_cur_changed()
	 * as ui_draw_* aren't called on just movements
	 */
	ui_draw_visual(buf);

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

static
void ui_draw_buf_1(buffer_t *buf, const rect_t *r)
{
	list_t *l;
	int y;

	buf->screen_coord = *r;

	for(y = 0, l = list_seek(buf->head, buf->ui_start.y, 0);
			l && y < r->h;
			l = l->next, y++)
	{
		const int lim = l->len_line < (unsigned)r->w
			? l->len_line
			: (unsigned)r->w;
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

	ui_draw_visual(buf);
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
