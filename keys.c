#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "pos.h"
#include "list.h"
#include "buffer.h"

#include "ui.h"
#include "motion.h"
#include "keys.h"
#include "ncurses.h"
#include "mem.h"
#include "cmds.h"

#include "buffers.h"

#include "config.h"

motionkey_t *motion_next(enum ui_mode mode, int ch, int count)
{
	extern motionkey_t motion_keys[];
	int i;

	for(i = 0; motion_keys[i].ch; i++){
		if(motion_keys[i].mode & mode
		&& motion_keys[i].ch == ch
		&& count-- < 0)
		{
			return &motion_keys[i];
		}
	}

	return NULL;
}

static
void parse_cmd(char *cmd, int *argc, char ***argv)
{
	char *p;

	for(p = strtok(cmd, " "); p; p = strtok(NULL, " ")){
		*argv = urealloc(*argv, (*argc + 2) * sizeof **argv);
		(*argv)[*argc] = ustrdup(p);
		++*argc;
	}
	if(*argc)
		(*argv)[*argc] = NULL;
}

void k_cmd(const keyarg_u *arg)
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

	if(!argc)
		goto cancel;

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

void k_redraw(const keyarg_u *a)
{
	(void)a;
	ui_redraw();
}

void k_set_mode(const keyarg_u *a)
{
	ui_mode = a->i;
}

void k_scroll(const keyarg_u *a)
{
	buffer_t *buf = buffers_cur();

	buf->ui_start.y += a->pos.y;

	if(buf->ui_start.y < 0)
		buf->ui_start.y = 0;

	if(buf->ui_pos.y < buf->ui_start.y)
		buf->ui_pos.y = buf->ui_start.y;
	else if(buf->ui_pos.y >= buf->ui_start.y + buf->screen_coord.h)
		buf->ui_pos.y = buf->ui_start.y + buf->screen_coord.h - 1;

	ui_redraw();
	ui_cur_changed();
}

void k_winsel(const keyarg_u *a)
{
	buffer_t *buf;
	char dir;

	(void)a;

	buf = buffers_cur();
	dir = nc_getch();

	switch(dir){
#define DIRECT(c, n) case c: buf = buf->neighbours[n]; break

		DIRECT('j', BUF_DOWN);
		DIRECT('k', BUF_UP);
		DIRECT('h', BUF_LEFT);
		DIRECT('l', BUF_RIGHT);

		default:
			return;
	}


	if(buf){
		buffers_set_cur(buf);
		ui_redraw();
		ui_cur_changed();
	}else{
		ui_status("no buffer");
	}
}

void k_show(const keyarg_u *a)
{
	buffer_t *buf = buffers_cur();
	(void)a;
	ui_status("%s%s%s, x=%d y=%d",
			buf->fname ? "\"" : "",
			buf->fname ? buf->fname : "<no name>",
			buf->fname ? "\"" : "",
			buf->ui_pos.x, buf->ui_pos.y);
}

void k_open(const keyarg_u *a)
{
	buffer_insline(buffers_cur(), a->i);
	ui_redraw();
	ui_cur_changed();
}

void k_del(const keyarg_u *a)
{
	motionkey_t *mk = motion_next(ui_mode, nc_getch(), 0);

	if(mk){
		//motion_apply(mk, buffers_cur());

		/* TODO: delete */
	}
}
