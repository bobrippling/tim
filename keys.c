#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "ui.h"
#include "motion.h"
#include "keys.h"
#include "ncurses.h"
#include "mem.h"
#include "cmds.h"

#include "list.h"
#include "pos.h"
#include "buffer.h"

#include "buffers.h"

#include "config.h"

void parse_cmd(char *cmd, int *argc, char ***argv)
{
	char *p;

	for(p = strtok(cmd, " "); p; p = strtok(NULL, " ")){
		*argv = urealloc(*argv, (*argc + 2) * sizeof **argv);
		(*argv)[*argc] = ustrdup(p);
		++*argc;
	}
	(*argv)[*argc] = NULL;
}

void k_cmd(const KeyArg *arg)
{
	int y, x;

	int reading = 1;
	int i = 0;
	int len = 10;
	char *cmd = umalloc(len);
	char **argv;
	int argc;

	(void)arg;

	nc_get_yx(&y, &x);

	nc_set_yx(nc_LINES() - 1, 0);
	nc_addch(':');
	nc_clrtoeol();

	argv = NULL;
	argc = 0;

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

	parse_cmd(cmd, &argc, &argv);

	for(i = 0; cmds[i].cmd; i++)
		if(!strcmp(cmds[i].cmd, cmd)){
			cmds[i].func(argc, argv);
			break;
		}

	if(!cmds[i].cmd)
		ui_status("unknown command %s", cmd);

cancel:
	free(cmd);
	for(i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
	nc_set_yx(y, x);
}

void k_redraw(const KeyArg *a)
{
	(void)a;
	ui_redraw();
}

void k_set_mode(const KeyArg *a)
{
	ui_mode = a->i;
}

void k_scroll(const KeyArg *a)
{
	const int nl = nc_LINES();
	buffer_t *buf = buffers_cur();

	buf->off_ui.y += a->pos.y;

	if(buf->off_ui.y < 0)
		buf->off_ui.y = 0;

	if(buf->pos_ui.y < buf->off_ui.y)
		buf->pos_ui.y = buf->off_ui.y;
	else if(buf->pos_ui.y >= buf->off_ui.y + nl)
		buf->pos_ui.y = buf->off_ui.y + nl - 2;

	ui_redraw();
	ui_cur_changed();
}

void k_winsel(const KeyArg *a)
{
	KeyArg cpy = *a;
	buffer_t *buf = buffers_cur();

	while(cpy.pos.x && buf)
		buf = buf->neighbours[cpy.pos.x-- > 0 ? BUF_RIGHT : BUF_LEFT];
	//while(cpy.pos.y && buf)
		//buf = buf->neighbours[cpy.pos.y > 0 ? BUF_DOWN  : BUF_UP];

	if(buf){
		buffers_set_cur(buf);
		ui_redraw();
		ui_cur_changed();
	}else{
		ui_status(":C");
	}
}
