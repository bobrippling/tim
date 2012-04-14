#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ui.h"
#include "ncurses.h"
#include "motion.h"
#include "keys.h"
#include "list.h"
#include "pos.h"
#include "buffer.h"
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

void ui_inschar(char ch)
{
	buffer_t *buf = buffers_cur();

	switch(ch){
		case CTRL_AND('?'):
		case CTRL_AND('H'):
		case 127:
			if(buf->pos_ui.x > 0)
				buffer_delchar(buffers_cur(), &buf->pos_ui.x, &buf->pos_ui.y);
			break;

		default:
			buffer_inschar(buffers_cur(), &buf->pos_ui.x, &buf->pos_ui.y, ch);
			break;
	}

	ui_cur_changed();
	ui_redraw();
}

void ui_main()
{
	extern Key keys[];
	extern MotionKey motion_keys[];

	ui_redraw();

	while(ui_running){
		int ch = nc_getch();
		int i;
		int found;

		for(found = i = 0; motion_keys[i].ch; i++)
			if(motion_keys[i].mode & ui_mode && motion_keys[i].ch == ch){
				motion_keys[i].func(&motion_keys[i].motion);
				found = 1;
			}

		for(i = 0; keys[i].ch; i++)
			if(keys[i].mode & ui_mode && keys[i].ch == ch){
				keys[i].func(&keys[i].arg);
				found = 1;
			}

		if(!found){
			if(ui_mode == UI_INSERT)
				ui_inschar(ch);
			else
				ui_status("unknown key %d", ch);
		}
	}
}

void ui_term()
{
	nc_term();
}

list_t *ui_current_line()
{
	buffer_t *buf = buffers_cur();
	return list_seek(buf->head, buf->pos_ui.y);
}

void ui_cur_changed()
{
	const int nl = nc_LINES();
	int need_redraw = 0;
	buffer_t *buf = buffers_cur();

	if(buf->pos_ui.y > buf->off_ui.y + nl - 2){
		buf->off_ui.y = buf->pos_ui.y - nl + 2;
		need_redraw = 1;
	}else if(buf->pos_ui.y < buf->off_ui.y){
		buf->off_ui.y = buf->pos_ui.y;
		need_redraw = 1;
	}

	if(buf->pos_ui.x < 0)
		buf->pos_ui.x = 0;

	nc_set_yx(buf->pos_screen.y + buf->pos_ui.y - buf->off_ui.y,
			      buf->pos_screen.x + buf->pos_ui.x - buf->off_ui.x);

	if(need_redraw)
		ui_redraw();

}

void ui_draw_buf_1(buffer_t *buf, rect_t *r)
{
	list_t *l;
	int y;

	buf->pos_screen.x = r->x;
	buf->pos_screen.y = r->y;

	for(y = 0, l = list_seek(buf->head, buf->off_ui.y); l && y < r->h; l = l->next, y++){
		int i;

		nc_set_yx(y, r->x);
		nc_clrtoeol();

		for(i = 0; i < l->len_line; i++)
			nc_addch(l->line[i]);
	}

	for(; y < r->h; y++){
		nc_set_yx(y, r->x);
		nc_addch('~');
		nc_clrtoeol();
	}
}

void ui_draw_buf_col(buffer_t *buf, rect_t *r_col)
{
	buffer_t *bi;
	int i;
	int h;
	int nrows;

	for(nrows = 1, bi = buf;
			bi->neighbours[BUF_DOWN];
			bi = bi->neighbours[BUF_DOWN], nrows++);

	h = nc_LINES() / nrows;

	for(i = 0; buf; buf = buf->neighbours[BUF_DOWN]){
		rect_t r = *r_col;
		r.y = i * nrows;
		r.h = h;
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

	w = nc_COLS() / ncols;

	for(i = 0; buf; i++, buf = buf->neighbours[BUF_RIGHT]){
		rect_t r;

		r.x = i * w;
		r.y = 0;
		r.w = w;
		r.h = nc_LINES();

		ui_draw_buf_col(buf, &r);
	}while(buf);

	nc_set_yx(save_y, save_x);
}
