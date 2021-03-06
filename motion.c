#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "window.h"
#include "motion.h"
#include "ui.h"
#include "io.h"
#include "str.h"
#include "prompt.h"
#include "word.h"
#include "macros.h"

#define UI_TOP(win) win->ui_start.y

enum
{
	MOTION_FAILURE = 0,
	MOTION_SUCCESS = 1
};

static void start_of_line_given(list_t *l, point_t *to)
{
	if(l){
		unsigned i;
		for(i = 0; i < l->len_line && isspace(l->line[i]); i++);

		to->x = i < l->len_line ? i : 0;
	}else{
		to->x = 0;
	}
}

void start_of_line(point_t *pt, window *w)
{
	if(!pt)
		pt = w->ui_pos;

	start_of_line_given(list_seek(w->buf->head, pt->y, false), pt);
}

static int m_line_goto(point_t *const pt, unsigned repeat, bool end, window *win)
{
	if(repeat == 0) /* default */
		pt->y = end ? list_count(win->buf->head) : 0;
	else
		pt->y = repeat - 1;

	start_of_line(pt, win);

	return MOTION_SUCCESS;
}

int m_eof(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	return m_line_goto(to, repeat, true, win);
}

int m_eol(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	list_t *l = window_current_line(win, false);

	to->x = l && l->len_line > 0 ? l->len_line - 1 : 0;

	bool nonblank = m->i;
	if(nonblank){
		for(; to->x > 0 && isspace(l->line[to->x]); to->x--);
	}

	return MOTION_SUCCESS;
}

int m_sos(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	to->y = UI_TOP(win);
	start_of_line(to, win);
	return MOTION_SUCCESS;
}

static void screen_or_buf_dimensions(
		window *win, int *const y, int *const height)
{
	int nlines = list_count(win->buf->head);

	*y = UI_TOP(win);

	if(nlines < win->screen_coord.h)
		*height = nlines - *y;
	else
		*height = win->screen_coord.h - 1;

	if(*height < 0)
		*height = 0;
}

int m_eos(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	int y, h;
	screen_or_buf_dimensions(win, &y, &h);
	to->y = y + h;

	start_of_line(to, win);
	return MOTION_SUCCESS;
}

int m_mos(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	int y, h;
	screen_or_buf_dimensions(win, &y, &h);
	to->y = y + h / 2;

	start_of_line(to, win);
	return MOTION_SUCCESS;
}

int m_goto(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	if(m->pos.y > -1)
		to->y = m->pos.y * DEFAULT_REPEAT(repeat);

	if(m->pos.x > -1)
		to->x = m->pos.x * DEFAULT_REPEAT(repeat);

	return MOTION_SUCCESS;
}

int m_move(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	to->y += m->pos.y * DEFAULT_REPEAT(repeat);
	to->x += m->pos.x * DEFAULT_REPEAT(repeat);
	if(to->x < 0 || to->y < 0)
		return MOTION_FAILURE;
	return MOTION_SUCCESS;
}

int m_sof(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	return m_line_goto(to, repeat, false, win);
}

int m_sol(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	list_t *l = window_current_line(win, false);

	start_of_line_given(l, to);
	return MOTION_SUCCESS;
}

int m_col(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	if(repeat == 0)
		to->x = 0;
	else
		to->x = repeat - 1;
	return MOTION_SUCCESS;
}

static int m_linesearch(
		motion_arg const *arg, unsigned repeat, window *win, point_t *to,
		list_t *sfn(motion_arg const *, list_t *, int *, const void *),
		const void *ctx)
{
	list_t *l = window_current_line(win, false); /* fine - repeat handled */
	int n = 0;

	*to = *win->ui_pos;

	if(!l)
		goto limit;

	for(repeat = DEFAULT_REPEAT(repeat);
			repeat > 0;
			repeat--)
	{
		l = sfn(arg, l, &n, ctx);
		if(!l)
			goto limit;
	}

	to->y += n;

	return MOTION_SUCCESS;
limit:
	to->y = arg->dir > 0 ? buffer_nlines(win->buf) : 0;
	return MOTION_SUCCESS;
}

static list_t *m_search_para(
		motion_arg const *a, list_t *l,
		int *pn, const void *ctx)
{
	/* while in space, find non-space */
	for(; l && (!l->line || isallspace(l->line, l->len_line));
			l = list_advance_y(l, a->dir, pn, NULL));

	/* while in non-space, find space */
	for(; l && (l->line && !isallspace(l->line, l->len_line));
			l = list_advance_y(l, a->dir, pn, NULL));

	return l;
}

int m_para(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	return m_linesearch(m, repeat, win, to, m_search_para, NULL);
}

static list_t *m_search_func(
		motion_arg const *a, list_t *l,
		int *pn, const void *ctx)
{
	int ch = *(int *)ctx;

	if(l && l->len_line && *l->line == ch)
		l = list_advance_y(l, a->dir, pn, NULL);

	for(;
	    l && (l->len_line == 0 || *l->line != ch);
	    l = list_advance_y(l, a->dir, pn, NULL));

	return l;
}

int m_func(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	int r = m_linesearch(m, repeat, win, to, m_search_func, &m->scan_ch);
	if(r == MOTION_SUCCESS)
		to->x = 0;
	return r;
}

static int m_word1(
		const buffer_t *buf, const int dir,
		const bool end, const bool big_words,
		point_t *to)
{
	list_t *l = list_seek(buf->head, to->y, false);

	enum word_state st = word_state(l, to->x, big_words);
	bool find_word = true;

	if(st & (W_KEYWORD | W_NON_KWORD)){
		/* we're on a word - if we're an end motion and not at the end,
		 * just need to move to the end */
		if(end == (dir > 0)){
			if(word_state(l, to->x + dir, big_words) != st){
				/* at the end - next */
			}else{
				/* seek to end and we're done */
				word_seek_to_end(l, dir, to, big_words);
				return MOTION_SUCCESS;
			}
		}

		/* skip to space or other word type */
		enum word_state other_word = ((W_KEYWORD | W_NON_KWORD) & ~st);

		l = word_seek(l, dir, to, W_SPACE | other_word, big_words);

		/* if we're on another word type, we're done */
		if(word_state(l, to->x, big_words) & other_word)
			find_word = false;
		else if(!l || !l->len_line)
			find_word = false; /* onto a new and empty - stop */
		/* else space - need to find the next word */

	}else{
		/* on space or none, go to next word */
	}

	if(find_word){
		l = word_seek(l, dir, to, W_KEYWORD | W_NON_KWORD, big_words);
		if(!l)
			return MOTION_FAILURE;
	}

	if(end == (dir > 0))
		word_seek_to_end(l, dir, to, big_words);

	return MOTION_SUCCESS;
}

int m_word(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	const int dir = m->word_type & WORD_BACKWARD ? -1 : 1;
	const bool end = m->word_type & WORD_END;
	const bool space = m->word_type & WORD_SPACE;

	for(repeat = DEFAULT_REPEAT(repeat); repeat > 0; repeat--)
		if(m_word1(win->buf, dir, end, space, to) == MOTION_FAILURE)
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

static int m_findnext2(const int ch, enum find_type ftype, unsigned repeat, window *win, point_t *to)
{
	list_t *l = window_current_line(win, false); /* fine - repeat handled */

	if(!l)
		return MOTION_FAILURE;

	point_t bpos = *win->ui_pos;

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

int m_findnext(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	return m_findnext2(
			last_find_ch,
			m->find_type ^ last_find_type,
			repeat,
			win,
			to);
}

int m_find(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	bool raw;
	return m_findnext2(
			last_find_ch = io_getch(IO_NOMAP /* vim doesn't mapraw here */, &raw, true),
			last_find_type = m->find_type,
			repeat, win, to);
}

static char *lastsearch;
static bool lastsearch_forward;

void m_setlastsearch(char *new, bool forward)
{
	free(lastsearch);
	lastsearch = new;
	lastsearch_forward = forward;
}

int m_searchnext(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	if(!lastsearch){
		ui_err("no last search");
		return MOTION_FAILURE;
	}

	const bool forward = m->i > 0;
	const int dir = (forward == lastsearch_forward) ? 1 : -1;

	repeat = DEFAULT_REPEAT(repeat);
	*to = *win->ui_pos;
	while(repeat --> 0){
		if(!buffer_findat(win->buf, lastsearch, to, dir)){
			ui_err("search pattern not found");
			return MOTION_FAILURE;
		}
	}

	ui_status("");
	return MOTION_SUCCESS;
}

int m_search(motion_arg const *m, unsigned repeat, window *win, point_t *to)
{
	const bool forward = m->i > 0;

	char *target = prompt(forward ? '/' : '?', NULL);
	if(!target)
		return MOTION_FAILURE;

	m_setlastsearch(target, forward);

	return m_searchnext(&(motion_arg){ 1 }, repeat, win, to);
}

int m_visual(
		motion_arg const *arg, unsigned repeat,
		window *win, point_t *to)
{
	(void)repeat;

	if(!(win->ui_mode & UI_VISUAL_ANY))
		return MOTION_FAILURE;

	/* line, char? */
	switch(win->ui_mode){
		case UI_NORMAL:
		case UI_INSERT: /* fall */
		case UI_INSERT_COL:
		case UI_VISUAL_CHAR: *arg->phow = M_NONE; break;
		case UI_VISUAL_LN:   *arg->phow = M_LINEWISE; break;
		case UI_VISUAL_COL:  *arg->phow = M_COLUMN; break;
	}

	/* set `to' to the opposite corner */
	*to = *window_uipos_alt(win);

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
		window *win, point_t *to)
{
	list_t *l = window_current_line(win, false);
	if(!l)
		return MOTION_FAILURE;

	if(arg->i == '%'){
		/* look forward for a paren on this line, then match */
		char paren = 0, opp;
		unsigned x = win->ui_pos->x;
		unsigned y = win->ui_pos->y;

		if(x >= l->len_line)
			x = l->len_line - 1;
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
		return MOTION_FAILURE;
	}

	if(paren_match(arg->i, &(char){ 0 })){
		repeat = DEFAULT_REPEAT(repeat);

		/* look for an unmatched `arg->i' */
		char paren = arg->i, opp = paren_opposite(paren);

		point_t pos = *win->ui_pos;

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

	return MOTION_FAILURE;
}

int motion_apply_win_dry(
		const motion *m, unsigned repeat,
		window *win, point_t *out)
{
	*out = *win->ui_pos;
	int r = m->func(&m->arg, repeat, win, out);
	if(r == MOTION_SUCCESS){
		if(out->y < 0 || out->x < 0)
			r = MOTION_FAILURE;
	}
	return r;
}

int motion_apply_win(const motion *m, unsigned repeat, window *win)
{
	point_t to;

	if(motion_apply_win_dry(m, repeat, win, &to)){
		if(memcmp(win->ui_pos, &to, sizeof to)){
			*win->ui_pos = to;
			ui_cur_changed();
		}
		return MOTION_SUCCESS;
	}
	return MOTION_FAILURE;
}

bool motion_to_region(
		const motion *m, unsigned repeat,
		window *win, region_t *out)
{
	region_t r = {
		.begin = *win->ui_pos
	};
	if(!motion_apply_win_dry(m, repeat, win, &r.end))
		return false;

	/* reverse if negative range */
	point_sort_yx(&r.begin, &r.end);

	if(m->how & M_COLUMN){
		r.type = REGION_COL;

		/* needs to be done before incrementing r.end.x/y below */
		point_sort_full(&r.begin, &r.end);

	}else if(m->how & M_LINEWISE){
		r.type = REGION_LINE;

	}else{
		r.type = REGION_CHAR;
	}

	if(win->ui_mode & UI_VISUAL_ANY){
		/* only increment y in the line case */
		r.end.x++;

	}else if(!(m->how & M_LINEWISE) && !(m->how & M_EXCLUSIVE)){
		/* linewise motions are always inclusive */
		++r.end.x;
	}

	*out = r;
	return true;
}
