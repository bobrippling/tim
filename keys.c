#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ui.h"
#include "keys.h"
#include "ncurses.h"
#include "mem.h"
#include "cmds.h"

#include "list.h"
#include "buffer.h"

#include "buffers.h"

#include "config.h"

void k_cmd(KeyArg *arg)
{
	int y, x;

	int reading = 1;
	int i = 0;
	int len = 10;
	char *cmd = umalloc(len);

	(void)arg;

	nc_get_yx(&y, &x);

	nc_set_yx(nc_LINES() - 1, 0);
	nc_addch(':');
	nc_clrtoeol();

	while(reading){
		int ch = nc_getch();

		switch(ch){
			case key_esc:
				goto cancel;

			case CTRL_AND('?'):
			case CTRL_AND('H'):
			case 263:
			case 127:
				/* backsapce */
				if(i == 0)
					goto cancel;
				cmd[i--] = '\0';
				nc_set_yx(nc_LINES() - 1, i + 1);
				break;

			case '\n':
			case '\r':
				reading = 0;
				break;

			default:
				cmd[i++] = ch;
				nc_addch(ch);
				if(i == len)
					cmd = urealloc(cmd, len *= 2);
				cmd[i] = '\0';
				break;
		}
	}

	for(i = 0; cmds[i].cmd; i++)
		if(!strcmp(cmds[i].cmd, cmd)){
			cmds[i].func();
			break;
		}

	if(!cmds[i].cmd)
		ui_status("unknown command %s", cmd);

cancel:
	free(cmd);
	nc_set_yx(y, x);
}

void k_down(KeyArg *a)
{
	(void)a;
	ui_y++;
	ui_cur_changed();
}

void k_up(KeyArg *a)
{
	(void)a;
	if(ui_y > 0){
		ui_y--;
		ui_cur_changed();
	}
}

void k_right(KeyArg *a)
{
	(void)a;
	ui_x++;
	ui_cur_changed();
}

void k_left(KeyArg *a)
{
	(void)a;
	if(ui_x > 0){
		ui_x--;
		ui_cur_changed();
	}
}

void k_redraw(KeyArg *a)
{
	(void)a;
	ui_redraw();
}

void k_set_mode(KeyArg *a)
{
	ui_mode = a->i;
}
