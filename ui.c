#include <stdlib.h>
#include <stdarg.h>

#include "ui.h"
#include "ncurses.h"
#include "keys.h"

int ui_running = 1;

void ui_init()
{
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

	while(ui_running){
		int ch = nc_getch();
		int i;

		for(i = 0; keys[i].ch; i++)
			if(keys[i].ch == ch){
				keys[i].func();
				break;
			}

		if(!keys[i].ch)
			ui_status("unknown key %d", ch);
	}
}

void ui_term()
{
	nc_term();
}
