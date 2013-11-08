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

#define UI_TOP(buf) buf->ui_start.y

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
	to->y = UI_TOP(buf);
	return MOTION_SUCCESS;
}

int m_eos(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = UI_TOP(buf) + buf->screen_coord.h - 1;
	return MOTION_SUCCESS;
}

int m_mos(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	to->y = UI_TOP(buf) + buf->screen_coord.h / 2 - 1;
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

static int m_linesearch(
		motion_arg const *arg, unsigned repeat, buffer_t *buf, point_t *to,
		list_t *sfn(motion_arg const *, list_t *, int *))
{
	list_t *l = buffer_current_line(buf);
	int n = 0;

	*to = *buf->ui_pos;

	if(!l)
		goto limit;

	for(repeat = DEFAULT_REPEAT(repeat);
			repeat > 0;
			repeat--)
	{
		l = sfn(arg, l, &n);
		if(!l)
			goto limit;
	}

	to->y += n;

	return MOTION_SUCCESS;
limit:
	to->y = arg->i > 0 ? buffer_nlines(buf) : 0;
	return MOTION_SUCCESS;
}

static list_t *m_search_para(motion_arg const *a, list_t *l, int *pn)
{
	/* while in space, find non-space */
	for(; l && (!l->line || isallspace(l->line));
			l = list_advance_y(l, a->i, pn, NULL));

	/* while in non-space, find space */
	for(; l && (l->line && !isallspace(l->line));
			l = list_advance_y(l, a->i, pn, NULL));

	return l;
}

int m_para(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	return m_linesearch(m, repeat, buf, to, m_search_para);
}

static list_t *m_search_func(motion_arg const *a, list_t *l, int *pn)
{
	if(l && l->len_line && *l->line == '{')
		l = list_advance_y(l, a->i, pn, NULL);

	for(;
	    l && (l->len_line == 0 || *l->line != '{');
	    l = list_advance_y(l, a->i, pn, NULL));

	return l;
}

int m_func(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	int r = m_linesearch(m, repeat, buf, to, m_search_func);
	if(r == MOTION_SUCCESS)
		to->x = 0;
	return r;
}

enum word_state
{
	W_NONE = 0,
	W_SPACE = 1 << 0,
	W_KEYWORD = 1 << 1,
	W_NON_KWORD = 1 << 2,
};

static enum word_state word_state(const list_t *l, int x)
{
	if(l && (unsigned)x < l->len_line){
		const char ch = l->line[x];

		if(isspace(ch))
			return W_SPACE;
		else if(isalnum(ch) || ch == '_')
			return W_KEYWORD;
		return W_NON_KWORD;
	}

	return W_NONE;
}

static list_t *word_seek(
		list_t *l, int dir, point_t *to,
		enum word_state target)
{
	while((word_state(l, to->x) & target) == 0){
		const int y = to->y;

		l = list_advance_x(l, dir, &to->y, &to->x);

		if(!l)
			break;
		if(y != to->y)
			break; /* newlines are a fresh start */
	}
	return l;
}

static int m_word1(const int dir, const buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	enum word_state st = word_state(l, to->x);
	bool find_word = true;

	if(st & (W_KEYWORD | W_NON_KWORD)){
		/* skip to space or other word type */
		enum word_state other_word = ((W_KEYWORD | W_NON_KWORD) & ~st);

		l = word_seek(l, dir, to, W_SPACE | other_word);

		/* if we're on another word type, we're done */
		if(word_state(l, to->x) & other_word)
			find_word = false;
		else if(!l || !l->len_line)
			find_word = false; /* onto a new and empty - stop */
		/* else space - need to find the next word */

	}else{
		/* on space or none, go to next word */
	}

	if(find_word && !(l = word_seek(l, dir, to, W_KEYWORD | W_NON_KWORD)))
		return MOTION_FAILURE;

	if(dir < 0){
		/* put us on the start of the word */
		const enum word_state st = word_state(l, to->x);

		while(to->x > 0 && word_state(l, to->x - 1) == st)
			to->x--;
	}
	return MOTION_SUCCESS;
}

int m_word(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	const int dir = m->i;

	for(repeat = DEFAULT_REPEAT(repeat); repeat > 0; repeat--)
		if(m_word1(dir, buf, to) == MOTION_FAILURE)
			return MOTION_FAILURE;

	return MOTION_SUCCESS;
}

static char *strchrdir(char *p, char ch, bool forward, char *start, size_t len)
{
	if(forward)
		return strchr_rev(p, ch, start);
	else
		return memchr(p, ch, len - (p - start));
}

static int m_findnext2(const int ch, enum find_type ftype, unsigned repeat, buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);

	if(!l)
		return MOTION_FAILURE;

	point_t bpos = *buf->ui_pos;

	if(!(ftype & F_REV)){
		bpos.x++;
		if((unsigned)bpos.x >= l->len_line)
			return MOTION_FAILURE;
	}else{
		if(l->len_line == 0)
			return MOTION_FAILURE;

		/* if we're after the actual length, move from logical to it */
		if((unsigned)bpos.x >= l->len_line)
			bpos.x = l->len_line;

		bpos.x--;

		if(bpos.x < 0)
			return MOTION_FAILURE;
	}

	repeat = DEFAULT_REPEAT(repeat);

	for(;;){
		char *p = l->line + bpos.x;

		p = strchrdir(p, ch, ftype & F_REV, l->line, l->len_line);

		if(!p)
			return MOTION_FAILURE;

		bpos.x = p - l->line;

		if(--repeat == 0){
			const int adj = ftype & F_TIL ? ftype & F_REV ? 1 : -1 : 0;

			*to = bpos;
			to->x += adj;
			break;
		}else{
			bpos.x += ftype & F_REV ? -1 : +1;
		}
	}

	return MOTION_SUCCESS;
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
			last_find_ch = io_getch(IO_NOMAP /* vim doesn't mapraw here */, NULL),
			last_find_type = m->find_type,
			repeat, buf, to);
}

static char *lastsearch;
int m_searchnext(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	if(!lastsearch){
		ui_status("no last search");
		return MOTION_FAILURE;
	}

	repeat = DEFAULT_REPEAT(repeat);
	*to = *buf->ui_pos;
	while(repeat --> 0){
		if(!buffer_findat(buf, lastsearch, to, m->i)){
			ui_status("search pattern not found");
			return MOTION_FAILURE;
		}
	}

	ui_status("");
	return MOTION_SUCCESS;
}

int m_search(motion_arg const *m, unsigned repeat, buffer_t *buf, point_t *to)
{
	char *target = prompt(m->i > 0 ? '/' : '?');
	if(!target)
		return MOTION_FAILURE;

	free(lastsearch);
	lastsearch = target;

	return m_searchnext(m, repeat, buf, to);
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
		case UI_INSERT_COL:
		case UI_VISUAL_CHAR: *arg->phow = M_NONE; break;
		case UI_VISUAL_LN:   *arg->phow = M_LINEWISE; break;
		case UI_VISUAL_COL:  *arg->phow = M_COLUMN; break;
	}

	/* set `to' to the opposite corner */
	*to = *buffer_uipos_alt(buf);

	return MOTION_SUCCESS;
}

static bool find_unnested_paren(
		const int ptofind, const int porig, const int dir,
		list_t **pl, point_t *pos)
{
	int nest = 0;

	for(list_t *l = *pl;
	    l;
	    l = *pl = list_advance_y(l, dir, &pos->y, &pos->x))
	{
		if((unsigned)pos->x >= l->len_line)
			continue;

		char in_quote = 0; /* scoped here - ignore multi-line quotes */

		char *p = l->line + pos->x;
		while(1){
			if((*p == '\'' || *p == '"')
			&& (!in_quote || in_quote == *p))
			{
				if(in_quote == *p){
					in_quote = 0;
				}else{
					in_quote = *p;
				}
			}else if(in_quote){
				/* skipping quotes */
			}else if(*p == ptofind){
				if(nest == 0){
					pos->x = p - l->line;
					return true;
				}
				nest--;
			}else if(*p == porig){
				nest++;
			}

			if(dir > 0){
				if(p == l->line + l->len_line - 1)
					break;
				p++;
			}else{
				if(p == l->line)
					break;
				p--;
			}
		}
	}

	return false;
}

int m_paren(
		motion_arg const *arg, unsigned repeat,
		buffer_t *buf, point_t *to)
{
	list_t *l = buffer_current_line(buf);
	if(!l)
		return MOTION_FAILURE;

	switch(arg->i){
		default:
			ui_status("bad m_paren arg");
			break;

		case '%':
		{
			/* look forward for a paren on this line, then match */
			char paren = 0, opp;
			unsigned x = buf->ui_pos->x;
			unsigned y = buf->ui_pos->y;

			if(x >= l->len_line)
				x = 0;
			for(unsigned i = x; i < l->len_line; i++)
				if(paren_match(l->line[i], &opp)){
					paren = l->line[i];
					x = i;
					break;
				}

			if(!paren)
				return MOTION_FAILURE;

			const int dir = paren_left(paren) ? 1 : -1;
			point_t pos = { .y = y, .x = x + dir };

			if(find_unnested_paren(opp, paren, dir, &l, &pos)){
				*to = pos;
				return MOTION_SUCCESS;
			}
			break;
		}

		case '{':
		case '(':
		case '}':
		case ')':
		{
			repeat = DEFAULT_REPEAT(repeat);

			/* look for an unmatched `arg->i' */
			char paren = arg->i, opp = paren_opposite(paren);

			point_t pos = *buf->ui_pos;

			int dir = paren_left(paren) ? -1 : 1;
			bool done_one = false;

			for(; repeat > 0; repeat--){
				const point_t before_move = pos;

				if((unsigned)pos.x < l->len_line &&
				(l->line[pos.x] == paren || l->line[pos.x] == opp))
				{
					/* "[{" while on a '{' - step out so we don't stay on it
					 * or
					 * "[{" while on a '}' - step in so we match the other paren
					 */
					l = list_advance_x(l, dir, &pos.y, &pos.x);
				}

				if(!find_unnested_paren(paren, opp,
							dir, &l, &pos))
				{
					if(done_one){
						pos = before_move;
						break;
					}
					return MOTION_FAILURE;
				}

				done_one = true;
			}

			*to = pos;
			return MOTION_SUCCESS;
		}
	}

	return MOTION_FAILURE;
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
