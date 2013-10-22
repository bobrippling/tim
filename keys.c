#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <wordexp.h>

#include "pos.h"
#include "list.h"
#include "buffer.h"

#include "ui.h"
#include "motion.h"
#include "keys.h"
#include "ncurses.h"
#include "mem.h"
#include "cmds.h"
#include "prompt.h"
#include "map.h"
#include "io.h"

#include "buffers.h"

#include "config.h"

static const motion *motion_find(int ch)
{
	for(int i = 0; motion_keys[i].ch; i++)
		if(motion_keys[i].ch == ch)
			return &motion_keys[i].motion;
	return NULL;
}

const motion *motion_read(unsigned *repeat)
{
	*repeat = io_read_repeat(IO_MAP);

	int ch = io_getch(IO_MAP);
	const motion *m = motion_find(ch);
	if(!m){
		io_ungetch(ch);
		return NULL;
	}

	return m;
}

static
const motion *motion_read_or_visual(unsigned *repeat)
{
	buffer_t *buf = buffers_cur();
	if(buf->ui_mode & UI_VISUAL_ANY){
		static motion visual = {
			.func = m_visual,
			.arg.phow = &visual.how
		};

		*repeat = 0;

		return &visual;
	}

	return motion_read(repeat);
}

static
char *parse_arg(const char *arg)
{
	/* TODO: ~ substitution */
	wordexp_t wexp;
	memset(&wexp, 0, sizeof wexp);

	int r = wordexp(arg, &wexp, WRDE_NOCMD);

	char *ret = r
		? ustrdup(arg)
		: join(" ", (const char **)wexp.we_wordv, wexp.we_wordc);

	wordfree(&wexp);

	return ret;
}

static
void parse_cmd(char *cmd, int *pargc, char ***pargv)
{
	int argc = *pargc;
	char **argv = *pargv;
	char *p;

	/* special case */
	if(ispunct(cmd[0])){
		argv = urealloc(argv, (argc + 2) * sizeof *argv);

		snprintf(
				argv[argc++] = umalloc(2),
				2, "%s", cmd);

		cmd++;
	}

	for(p = strtok(cmd, " "); p; p = strtok(NULL, " ")){
		argv = urealloc(argv, (argc + 2) * sizeof *argv);
		argv[argc++] = parse_arg(p);
	}
	if(argc)
		argv[argc] = NULL;

	*pargv = argv;
	*pargc = argc;
}

static
void filter_cmd(int *pargc, char ***pargv)
{
	/* check for '%' */
	int argc = *pargc;
	char **argv = *pargv;
	int i;
	const char *const fnam = buffer_fname(buffers_cur());


	for(i = 0; i < argc; i++){
		char *p;

		for(p = argv[i]; *p; p++){
			if(*p == '\\')
				continue;

			switch(*p){
				/* TODO: '#' */
				case '%':
					if(fnam){
						const int di = p - argv[i];
						char *new;

						*p = '\0';

						new = join("", (const char *[]){
								argv[i],
								fnam,
								p + 1 }, 3);

						free(argv[i]);
						argv[i] = new;
						p = argv[i] + di;
					}
					break;
			}
		}
	}
}

void k_cmd(const keyarg_u *arg, unsigned repeat, const int from_ch)
{
	int i;
	char **argv = NULL;
	int argc = 0;
	char *cmd = prompt(':');

	if(!cmd)
		goto cancel;

	parse_cmd(cmd, &argc, &argv);

	filter_cmd(&argc, &argv);

	if(!argc)
		goto cancel;

	for(i = 0; cmds[i].cmd; i++)
		if(!strcmp(cmds[i].cmd, argv[0])){
			cmds[i].func(argc, argv);
			break;
		}

	if(!cmds[i].cmd)
		ui_status("unknown command %s", argv[0]);

cancel:
	free(cmd);
	for(i = 0; i < argc; i++)
		free(argv[i]);
	free(argv);
}

void k_redraw(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	(void)a;
	ui_clear();
	ui_redraw();
}

void k_set_mode(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	ui_set_bufmode(a->i);
}

void k_scroll(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();

	buf->ui_start.y += a->pos.y;

	if(buf->ui_start.y < 0)
		buf->ui_start.y = 0;

	if(buf->ui_pos->y < buf->ui_start.y)
		buf->ui_pos->y = buf->ui_start.y;
	else if(buf->ui_pos->y >= buf->ui_start.y + buf->screen_coord.h)
		buf->ui_pos->y = buf->ui_start.y + buf->screen_coord.h - 1;

	ui_redraw();
	ui_cur_changed();
}

void k_winsel(const keyarg_u *a, unsigned repeat, const int from_ch)
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

void k_show(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_t *buf = buffers_cur();
	(void)a;
	ui_status("%s%s%s, x=%d y=%d",
			buf->fname ? "\"" : "",
			buf->fname ? buffer_shortfname(buf->fname) : "<no name>",
			buf->fname ? "\"" : "",
			buf->ui_pos->x, buf->ui_pos->y);
}

void k_open(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_insline(buffers_cur(), a->i);
	ui_set_bufmode(UI_INSERT);
	ui_redraw();
	ui_cur_changed();
}

void k_case(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	repeat = DEFAULT_REPEAT(repeat);

	buffer_case(buffers_cur(), a->i, repeat);

	ui_redraw();
	ui_cur_changed();
}

void k_replace(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	if(a->i == 1){
		// TODO: repeated
	}else{
		/* single char */
		int ch = io_getch(IO_NOMAP);

		if(ch == K_ESC)
			return;

		buffer_replace_chars(
				buffers_cur(),
				ch,
				DEFAULT_REPEAT(repeat));
	}
	ui_redraw();
	ui_cur_changed();
}

void k_motion(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	motion_apply_buf(&a->motion.m, a->motion.repeat, buffers_cur());
}

static int around_motion(
		const keyarg_u *a, unsigned repeat, const int from_ch,
		buffer_action *action)
{
	motion m_doubletap = {
		.func = m_move,
		.how = M_LINEWISE,
	};

	unsigned repeat_motion;
	const motion *m = motion_read_or_visual(&repeat_motion);

	if(!m){
		/* check for dd, etc */
		int ch = io_getch(IO_NOMAP);
		if(ch == from_ch){
			/* dd - stay where we are, +the repeat */
			m_doubletap.arg.pos.y = repeat - 1;
			repeat = 0;
			m = &m_doubletap;
		}else{
			if(ch != K_ESC)
				ui_status("no motion '%c'", ch);
			/*io_ungetch(ch);*/
		}
	}

	if(m){
		repeat = DEFAULT_REPEAT(repeat) * DEFAULT_REPEAT(repeat_motion);

		buffer_t *b = buffers_cur();
		point_t to;
		if(!motion_apply_buf_dry(m, repeat, b, &to))
			return 0;

		point_t from = *b->ui_pos;

		/* reverse if negative range */
		if(to.y < from.y){
			point_t tmp = to;
			to = from;
			from = tmp;
		}else if(to.y == from.y && to.x < from.x){
			int tmp = to.x;
			to.x = from.x;
			from.x = tmp;
		}

		if(!(m->how & M_EXCLUSIVE))
			m->how & M_LINEWISE ? ++to.y : ++to.x;

		enum list_region r;
		if(m->how & M_COLUMN)
			r = HOW_COL;
		else if(m->how & M_LINEWISE)
			r = HOW_LINE;
		else
			r = HOW_CHAR;

		action(b, &from, &to, r);

		*b->ui_pos = from;

		ui_set_bufmode(UI_NORMAL);

		ui_redraw();
		ui_cur_changed();

		return 1;
	}

	return 0;
}

void k_del(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion(a, repeat, from_ch, buffer_delbetween);
}

void k_change(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	if(around_motion(a, repeat, from_ch, buffer_delbetween))
		k_set_mode(&(keyarg_u){ UI_INSERT }, 0, 0);
}

void k_join(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion(a, repeat, from_ch, buffer_joinbetween);
}

void k_indent(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	around_motion(a, repeat, from_ch,
			a->i > 0 ? buffer_indent : buffer_unindent);
}

void k_vtoggle(const keyarg_u *a, unsigned repeat, const int from_ch)
{
	buffer_togglev(buffers_cur());
	ui_cur_changed();
}
