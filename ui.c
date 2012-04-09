#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ui.h"
#include "ncurses.h"
#include "keys.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"

enum ui_mode ui_mode;
int ui_running = 1;
int ui_x, ui_y;

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

void ui_main()
{
	extern Key keys[];

	ui_x = ui_y = 0;

	ui_redraw();

	while(ui_running){
		int ch = nc_getch();
		int i;
		int found;

		for(i = found = 0; keys[i].ch; i++)
			if(keys[i].mode & ui_mode && keys[i].ch == ch){
				keys[i].func(&keys[i].arg);
				found = 1;
			}


		if(!found){
			if(ui_mode == UI_INSERT){
				buffer_inschar(buffers_cur(), ui_x, ui_y, ch);
				ui_x++;
				ui_redraw();
			}else{
				ui_status("unknown key %d", ch);
			}
		}
	}
}

void ui_term()
{
	nc_term();
}

void ui_cur_changed()
{
	/* TODO: scroll, etc */
	nc_set_yx(ui_y, ui_x);
}

void ui_redraw()
{
	const int nl = nc_LINES();
	list_t *l;
	int y = 0;

	nc_cls();

	for(l = buffers_cur()->head; l && y < nl; l = l->next, y++){
		int i;

		nc_set_yx(y, 0);

		for(i = 0; i < l->len_line; i++)
			nc_addch(l->line[i]);
	}

	nc_set_yx(y, 0);
	for(; y < nl; y++){
		nc_set_yx(y, 0);
		nc_addch('~');
	}

	nc_set_yx(ui_y, ui_x);
}
