#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "pos.h"
#include "list.h"
#include "buffer.h"
#include "motion.h"
#include "ui.h"

#define UI_X   buf->ui_pos.x
#define UI_Y   buf->ui_pos.y
#define UI_TOP buf->ui_start.y


void m_eof(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	to->y = list_count(buf->head);
}

void m_eol(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	to->x = l ? l->len_line - 1 : 0;
}

void m_sos(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP;
}

void m_eos(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h - 1;
}

void m_mos(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h / 2 - 1;
}

void m_goto(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	if(m->pos.y > -1)
		to->y = m->pos.y;

	if(m->pos.x > -1)
		to->x = m->pos.x;
}

void m_move(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	to->y += m->pos.y;
	to->x += m->pos.x;
}

void m_sof(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	to->y = 0;
}

void m_sol(motion_arg const *m, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);
	unsigned int i;

	if(l){
		for(i = 0; i < l->len_line && isspace(l->line[i]); i++);

		to->x = i < l->len_line ? i : 0;
	}else{
		to->x = 0;
	}
}

void motion_apply_buf_dry(const motion *m, const buffer_t *buf, point_t *out)
{
	*out = buf->ui_pos;
	m->func(&m->arg, buf, out);
}

void motion_apply_buf(const motion *m, buffer_t *buf)
{
	point_t to;

	motion_apply_buf_dry(m, buf, &to);

	if(memcmp(&buf->ui_pos, &to, sizeof to)){
		buf->ui_pos = to;
		ui_cur_changed();
	}
}
