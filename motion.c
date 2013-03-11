#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "pos.h"
#include "list.h"
#include "buffer.h"
#include "motion.h"
#include "ui.h"
#include "ncurses.h"

#define UI_X   buf->ui_pos.x
#define UI_Y   buf->ui_pos.y
#define UI_TOP buf->ui_start.y

void m_eof(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = list_count(buf->head);
}

void m_eol(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	to->x = l ? l->len_line - 1 : 0;
}

void m_sos(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP;
}

void m_eos(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h - 1;
}

void m_mos(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h / 2 - 1;
}

void m_goto(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	/* TODO: repeat */
	if(m->pos.y > -1)
		to->y = m->pos.y;

	if(m->pos.x > -1)
		to->x = m->pos.x;
}

void m_move(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y += m->pos.y * DEFAULT_REPEAT(repeat);
	to->x += m->pos.x * DEFAULT_REPEAT(repeat);
}

void m_sof(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = 0;
}

void m_sol(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
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

void m_find(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	int f = nc_getch();
	struct list *l = buffer_current_line(buf);

	if(!l)
		goto failed;

	point_t bpos = buf->ui_pos;
	bpos.x++;
	if((unsigned)bpos.x >= l->len_line)
		goto failed;

	char *start = l->line + bpos.x;
	char *p = strchr(start, f);
	if(!p)
		goto failed;

	*to = (point_t){ .x = bpos.x + p - start,
	                 .y = bpos.y };
	return;

failed: /* FIXME: need to say motion failed */
	;
}

void motion_apply_buf_dry(const motion_repeat *mr, const buffer_t *buf, point_t *out)
{
	*out = buf->ui_pos;
	mr->motion->func(&mr->motion->arg, mr->repeat, buf, out);
}

void motion_apply_buf(const motion_repeat *mr, buffer_t *buf)
{
	point_t to;

	motion_apply_buf_dry(mr, buf, &to);

	if(memcmp(&buf->ui_pos, &to, sizeof to)){
		buf->ui_pos = to;
		ui_cur_changed();
	}
}
