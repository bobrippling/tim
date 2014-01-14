#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "hash.h"

#include "pos.h"
#include "ncurses.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "ui.h"
#include "motion.h"
#include "io.h"
#include "keys.h"
#include "buffers.h"
#include "mem.h"
#include "complete.h"
#include "keymacros.h"

#define UI_MODE() buffers_cur()->ui_mode

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

int ui_main()
{
	extern nkey_t nkeys[];
	extern const size_t nkeys_cnt;

	ui_redraw(); /* this first, to frame buf_sel */
	ui_cur_changed(); /* this, in case there's an initial buf offset */

	while(1){
		buffer_t *buf = buffers_cur();

		if(UI_MODE() & UI_VISUAL_ANY){
			point_t *alt = buffer_uipos_alt(buf);

			ui_rstatus("%d,%d - %d,%d",
					buf->ui_pos->y, buf->ui_pos->x,
					alt->y, alt->x);
		}

		const bool ins = UI_MODE() & UI_INSERT_ANY;
		unsigned repeat = 0;

		if(!ins){
			const motion *m = motion_read(&repeat, /*map:*/true);

			if(m){
				motion_apply_buf(m, repeat, buf);
				continue;
			}
		}

		const enum io io_mode = bufmode_to_iomap(buf->ui_mode);
		bool wasraw;
		int ch = io_getch(io_mode | IO_MAPRAW, &wasraw);

		if(ch == -1) /* eof */
			ui_run = UI_EXIT_1;

		bool found = false;
		if(!wasraw){
			io_ungetch(ch);

			int i = keys_filter(
					io_mode, (char *)nkeys, nkeys_cnt,
					offsetof(nkey_t, str), sizeof(nkey_t),
					offsetof(nkey_t, mode));

			if(i != -1){
				nkeys[i].func(&nkeys[i].arg, repeat, nkeys[i].str[0]);
				found = true;
			}else{
				ch = io_getch(0, NULL); /* pop it back */
			}
		}

		if(!found){
			if(UI_MODE() & UI_INSERT_ANY){
				buffer_inschar(buf, ch);
				ui_redraw();
				ui_cur_changed();
			}else if(ch != K_ESC){
				ui_err("unknown key %c", ch);
			}
		}

		switch(ui_run){
			case UI_RUNNING: continue;
			case UI_EXIT_0: return 0;
			case UI_EXIT_1: return 1;
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
			if(buf->ui_mode & UI_VISUAL_ANY)
				nc_highlight(region_contains(
							&hlregion, x, buf->ui_start.y + y));

			nc_addch(l->line[x]);
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

static void ui_complete_line(
		const point_t *at,
		const char *s, bool selected,
		size_t padding)
{
	nc_highlight(true);
	nc_set_yx(at->y, at->x);

	const int xmax = nc_COLS();
	for(int i = 0; s[i] && i < xmax; i++, padding--)
		nc_addch(s[i]);

	while(padding --> 0)
		nc_addch(' ');

	nc_highlight(false);
}

#define COMPL_MAX 10
void ui_draw_completion(
		struct hash *ents, const int sel,
		point_t const *at,
		const size_t len,
		bool is_hidden(void *),
		char *get_str(void *))
{
	point_t nc_orig;
	nc_get_yx(&nc_orig.y, &nc_orig.x);

	int start = sel - (COMPL_MAX / 2);
	if(start < 0)
		start = 0;

	size_t longest = 0;

	char *p;
	for(int i = start; (p = hash_ent(ents, i)); i++){
		if(!is_hidden(p)){
			size_t l = strlen(get_str(p));
			if(l > longest)
				longest = l;
		}
	}

	const int lastline = nc_LINES() - 1;
	int count = 0;

	for(int i = start; (p = hash_ent(ents, i)); i++){
		const char *ent_str = get_str(p);

		if(is_hidden(p) || strlen(ent_str) <= len)
			continue;

		point_t where = {
			.y = at->y + count + 1,
			.x = at->x
		};

		if(count == COMPL_MAX || where.y == lastline){
			ui_complete_line(&where, "...", false, longest - len);
		}else{
			ui_complete_line(&where, ent_str + len, i == sel, longest - len);
		}

		count++;
	}

	nc_set_yx(nc_orig.y, nc_orig.x);
}
