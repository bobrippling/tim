#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "list.h"
#include "pos.h"
#include "buffer.h"
#include "buffers.h"
#include "motion.h"
#include "ui.h"
#include "ncurses.h"

#define ui_x   buffers_cur()->pos_ui.x
#define ui_y   buffers_cur()->pos_ui.y
#define ui_top buffers_cur()->off_ui.y

void m_eof(Motion *m)
{
	(void)m;
	ui_y = list_count(buffers_cur()->head);
	ui_cur_changed();
}

void m_eol(Motion *m)
{
	list_t *l = ui_current_line();

	(void)m;

	ui_x = l->len_line - 1;

	ui_cur_changed();
}

void m_sos(Motion *m)
{
	(void)m;

	ui_y = ui_top;

	ui_cur_changed();
}

void m_eos(Motion *m)
{
	(void)m;

	ui_y = ui_top + nc_LINES() - 2;

	ui_cur_changed();
}

void m_mos(Motion *m)
{
	(void)m;

	ui_y = ui_top + nc_LINES() / 2 - 1;

	ui_cur_changed();
}

void m_goto(Motion *m)
{
	if(m->pos.y > -1)
		ui_y = m->pos.y;

	if(m->pos.x > -1)
		ui_x = m->pos.x;

	ui_cur_changed();
}

void m_move(Motion *m)
{
	(void)m;

	ui_y += m->pos.y;
	ui_x += m->pos.x;

	ui_cur_changed();
}

void m_sof(Motion *m)
{
	(void)m;

	ui_y = 0;

	ui_cur_changed();
}

void m_sol(Motion *m)
{
	list_t *l = ui_current_line();
	int i;

	(void)m;

	for(i = 0; i < l->len_line && isspace(l->line[i]); i++);

	ui_x = i < l->len_line ? i : 0;

	ui_cur_changed();
}
