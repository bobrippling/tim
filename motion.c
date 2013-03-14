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
#include "str.h"

#define UI_X   buf->ui_pos.x
#define UI_Y   buf->ui_pos.y
#define UI_TOP buf->ui_start.y

enum
{
	MOTION_FAILURE = 0,
	MOTION_SUCCESS = 1
};

int m_eof(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = list_count(buf->head);
	return MOTION_SUCCESS;
}

int m_eol(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	to->x = l ? l->len_line - 1 : 0;
	return MOTION_SUCCESS;
}

int m_sos(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP;
	return MOTION_SUCCESS;
}

int m_eos(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h - 1;
	return MOTION_SUCCESS;
}

int m_mos(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h / 2 - 1;
	return MOTION_SUCCESS;
}

int m_goto(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	if(m->pos.y > -1)
		to->y = m->pos.y * DEFAULT_REPEAT(repeat);

	if(m->pos.x > -1)
		to->x = m->pos.x * DEFAULT_REPEAT(repeat);

	return MOTION_SUCCESS;
}

int m_move(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y += m->pos.y * DEFAULT_REPEAT(repeat);
	to->x += m->pos.x * DEFAULT_REPEAT(repeat);
	if(to->x < 0) to->x = 0;
	if(to->y < 0) to->y = 0;
	return MOTION_SUCCESS;
}

int m_sof(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	to->y = 0;
	return MOTION_SUCCESS;
}

int m_sol(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);
	unsigned int i;

	if(l){
		for(i = 0; i < l->len_line && isspace(l->line[i]); i++);

		to->x = i < l->len_line ? i : 0;
	}else{
		to->x = 0;
	}
	return MOTION_SUCCESS;
}

int m_para(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);
	unsigned n = 0;

	/* TODO: reverse */

	if(!l)
		return MOTION_FAILURE;

	for(repeat = DEFAULT_REPEAT(repeat);
			repeat > 0;
			repeat--)
	{
		/* while in space, find non-space */
		for(; !l->line || isallspace(l->line); l = l->next, n++);

		/* while in non-space, find space */
		for(; l->line && !isallspace(l->line); l = l->next, n++);
	}

	*to = buf->ui_pos;
	to->y += n;

	return MOTION_SUCCESS;
}

static int m_findnext2(const int ch, enum find_type ftype, unsigned repeat, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	if(!l)
		goto failed;

	point_t bpos = buf->ui_pos;

	if(!(ftype & F_REV)){
		bpos.x++;
		if((unsigned)bpos.x >= l->len_line)
			goto failed;
	}else{
		if(l->len_line == 0)
			goto failed;

		/* if we're after the actual length, move from logical to it */
		if((unsigned)bpos.x >= l->len_line)
			bpos.x = l->len_line;

		bpos.x--;

		if(bpos.x < 0)
			goto failed;
	}

	char *const start = l->line + bpos.x;
	char *p = start;

	repeat = DEFAULT_REPEAT(repeat);

	for(;;){
		p = (ftype & F_REV ? strchr_rev(p, ch, l->line) : strchr(p, ch));

		if(!p)
			goto failed;

		if(--repeat == 0){
			const int adj = ftype & F_TIL ? ftype & F_REV ? 1 : -1 : 0;

			*to = (point_t){ .x = bpos.x + p - start + adj,
				               .y = bpos.y };
			break;
		}else{
			p++;
		}
	}

	return MOTION_SUCCESS;
failed:
	return MOTION_FAILURE;
}

static int last_find_ch;
static enum find_type last_find_type;

int m_findnext(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	return m_findnext2(
			last_find_ch,
			m->find_type ^ last_find_type,
			repeat,
			buf,
			to);
}

int m_find(motion_arg const *m, unsigned repeat, const buffer_t *buf, point_t *to)
{
	return m_findnext2(
			last_find_ch = nc_getch(),
			last_find_type = m->find_type,
			repeat, buf, to);
}

int motion_apply_buf_dry(const motion_repeat *mr, const buffer_t *buf, point_t *out)
{
	*out = buf->ui_pos;
	return mr->motion->func(&mr->motion->arg, mr->repeat, buf, out);
}

int motion_apply_buf(const motion_repeat *mr, buffer_t *buf)
{
	point_t to;

	if(motion_apply_buf_dry(mr, buf, &to)){
		if(memcmp(&buf->ui_pos, &to, sizeof to)){
			buf->ui_pos = to;
			ui_cur_changed();
		}
		return MOTION_SUCCESS;
	}
	return MOTION_FAILURE;
}
