#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ui.h"
#include "ncurses.h"
#include "motion.h"
#include "keys.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"

enum ui_mode ui_mode;
int ui_running = 1;
int ui_x, ui_y;
int ui_top;

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
	switch(ch){
		case CTRL_AND('?'):
		case CTRL_AND('H'):
		case 127:
			if(ui_x > 0)
				buffer_delchar(buffers_cur(), &ui_x, &ui_y);
			break;

		default:
			buffer_inschar(buffers_cur(), &ui_x, &ui_y, ch);
			break;
	}

	ui_cur_changed();
	ui_redraw();
}

void ui_main()
{
	extern Key keys[];
	extern MotionKey motion_keys[];

	ui_x = ui_y = ui_top = 0;

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

struct list *ui_current_line()
{
	return list_seek(buffers_cur()->head, ui_y);
}

void ui_cur_changed()
{
	const int nl = nc_LINES();
	int need_redraw = 0;

	if(ui_y > ui_top + nl - 2){
		ui_top = ui_y - nl + 2;
		need_redraw = 1;
	}else if(ui_y < ui_top){
		ui_top = ui_y;
		need_redraw = 1;
	}

	if(ui_x < 0)
		ui_x = 0;

	nc_set_yx(ui_y - ui_top, ui_x);

	if(need_redraw)
		ui_redraw();
}

void ui_redraw()
{
	const int nl = nc_LINES() - 1;
	list_t *l;
	int save_y, save_x;
	int y;

	nc_get_yx(&save_y, &save_x);

	for(y = 0, l = list_seek(buffers_cur()->head, ui_top); l && y < nl; l = l->next, y++){
		int i;

		nc_set_yx(y, 0);
		nc_clrtoeol();

		for(i = 0; i < l->len_line; i++)
			nc_addch(l->line[i]);
	}

	for(; y < nl; y++){
		nc_set_yx(y, 0);
		nc_addch('~');
		nc_clrtoeol();
	}

	nc_set_yx(save_y, save_x);
}
