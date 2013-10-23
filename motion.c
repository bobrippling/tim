#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "motion.h"
#include "ui.h"
#include "io.h"
#include "str.h"
#include "prompt.h"

#define UI_X   buf->ui_pos.x
#define UI_Y   buf->ui_pos.y
#define UI_TOP buf->ui_start.y

enum
{
	MOTION_FAILURE = 0,
	MOTION_SUCCESS = 1
};

int m_eof(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = list_count(buf->head);
	return MOTION_SUCCESS;
}

int m_eol(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	to->x = l && l->len_line > 0 ? l->len_line - 1 : 0;
	return MOTION_SUCCESS;
}

int m_sos(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = UI_TOP;
	return MOTION_SUCCESS;
}

int m_eos(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h - 1;
	return MOTION_SUCCESS;
}

int m_mos(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = UI_TOP + buf->screen_coord.h / 2 - 1;
	return MOTION_SUCCESS;
}

int m_goto(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	if(m->pos.y > -1)
		to->y = m->pos.y * DEFAULT_REPEAT(repeat);

	if(m->pos.x > -1)
		to->x = m->pos.x * DEFAULT_REPEAT(repeat);

	return MOTION_SUCCESS;
}

int m_move(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y += m->pos.y * DEFAULT_REPEAT(repeat);
	to->x += m->pos.x * DEFAULT_REPEAT(repeat);
	if(to->x < 0 || to->y < 0)
		return MOTION_FAILURE;
	return MOTION_SUCCESS;
}

int m_sof(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = 0;
	return MOTION_SUCCESS;
}

int m_sol(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
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

static list_t *advance_line(list_t *l, unsigned *pn, const int dir)
{
	*pn += dir;
	return dir > 0 ? l->next : l->prev;
}

int m_para(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);
	unsigned n = 0;

	*to = *buf->ui_pos;

	if(!l)
		goto limit;

	for(repeat = DEFAULT_REPEAT(repeat);
			repeat > 0;
			repeat--)
	{
		/* while in space, find non-space */
		for(; l && (!l->line || isallspace(l->line)); l = advance_line(l, &n, m->i));

		/* while in non-space, find space */
		for(; l && (l->line && !isallspace(l->line)); l = advance_line(l, &n, m->i));

		if(!l)
			goto limit;
	}

	to->y += n;

	return MOTION_SUCCESS;
limit:
	to->y = m->i > 0 ? buffer_nlines(buf) : 0;
	return MOTION_SUCCESS;
}

enum word_state
{
	W_WORD, W_PUNCT, W_SPACE, W_NONE
};

static enum word_state word_state(const list_t *l, int x)
{
	if(l && (unsigned)x < l->len_line){
		const char ch = l->line[x];

		if(isspace(ch))
			return W_SPACE;
		else if(isalnum(ch) || ch == '_')
			return W_WORD;
		else if(ispunct(ch))
			return W_PUNCT;
	}

	return W_NONE;
}

static int word_state_eq(enum word_state a, enum word_state b)
{
	/* treat punct and space as equal for moving around */
	return (a == W_WORD) == (b == W_WORD);
}

static int m_word1(const int dir, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	const enum word_state state_1 = word_state(l, to->x);

	/* skip until the state changes */
	for(;;){
		if(!l)
			return MOTION_FAILURE;

		to->x += dir;

		/* bounds check */
		if(dir > 0 ? (unsigned)to->x >= l->len_line : to->x < 0){
			to->y += dir;
			if(dir < 0){
				if(to->y < 0)
					return MOTION_FAILURE;

				l = l->prev;
				to->x = l->len_line ? l->len_line - 1 : 0;
			}else{
				l = l->next;
				to->x = 0;
			}
		}

		if(!word_state_eq(word_state(l, to->x), state_1)){
			if(dir < 0){
				enum word_state st = word_state(l, to->x);
				while(to->x > 0 && word_state(l, to->x - 1) == st)
					to->x--;
			}

			return MOTION_SUCCESS;
		}
	}
}

int m_word(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	const int dir = m->i;

	for(repeat = DEFAULT_REPEAT(repeat); repeat > 0; repeat--)
		if(m_word1(dir, buf, to) == MOTION_FAILURE)
			return MOTION_FAILURE;

	return MOTION_SUCCESS;
}

static int m_findnext2(const int ch, enum find_type ftype, unsigned repeat, buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	if(!l)
		goto failed;

	point_t bpos = *buf->ui_pos;

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

int m_findnext(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	return m_findnext2(
			last_find_ch,
			m->find_type ^ last_find_type,
			repeat,
			buf,
			to);
}

int m_find(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	return m_findnext2(
			last_find_ch = io_getch(IO_NOMAP),
			last_find_type = m->find_type,
			repeat, buf, to);
}

int m_search(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	char *target = prompt(m->i ? '?' : '/');
	if(!target)
		return MOTION_FAILURE;

	int found = buffer_find(buf, target, to, m->i);
	free(target);
	return found;
}

int m_visual(
		motion_arg const *arg, unsigned repeat,
		buffer_t *buf, point_t *to)
{
	(void)repeat;

	if(!(buf->ui_mode & UI_VISUAL_ANY))
		return MOTION_FAILURE;

	/* line, char? */
	switch(buf->ui_mode){
		case UI_NORMAL:
		case UI_INSERT: /* fall */
		case UI_VISUAL_CHAR: *arg->phow = M_NONE; break;
		case UI_VISUAL_LN:   *arg->phow = M_LINEWISE; break;
		case UI_VISUAL_COL:  *arg->phow = M_COLUMN; break;
	}

	/* set `to' to the opposite corner */
	*to = *buffer_uipos_alt(buf);

	return MOTION_SUCCESS;
}

int motion_apply_buf_dry(
		const motion *m, unsigned repeat,
		buffer_t *buf, point_t *out)
{
	*out = *buf->ui_pos;
	return m->func(&m->arg, repeat, buf, out);
}

int motion_apply_buf(const motion *m, unsigned repeat, buffer_t *buf)
{
	point_t to;

	if(motion_apply_buf_dry(m, repeat, buf, &to)){
		if(memcmp(buf->ui_pos, &to, sizeof to)){
			*buf->ui_pos = to;
			ui_cur_changed();
		}
		return MOTION_SUCCESS;
	}
	return MOTION_FAILURE;
}
