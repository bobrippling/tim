#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "mem.h"
#include "yank.h"
#include "macros.h"
#include "ncurses.h"
#include "str.h"
#include "retain.h"

static
int buffer_replace_file(buffer_t *b, FILE *f);

void buffer_free(buffer_t *b)
{
	list_free(b->head);
	free(b->fname);
	free(b);
}

buffer_t *buffer_new()
{
	buffer_t *b = umalloc(sizeof *b);
	b->head = list_new(NULL);
	b->ui_pos = &b->ui_npos;
	b->ui_mode = UI_NORMAL;
	b->eol = true; /* default to nice eol */
	b->ui_paren = (point_t){ -1, -1 };
	return b;
}

void buffer_togglev(buffer_t *buf, bool corner_toggle)
{
	if(corner_toggle){
		point_t *alt = buffer_uipos_alt(buf);

		int tmp = buf->ui_pos->x;
		buf->ui_pos->x = alt->x;
		alt->x = tmp;
	}else{
		buf->ui_pos = (buf->ui_pos == &buf->ui_npos)
			? &buf->ui_vpos
			: &buf->ui_npos;
	}
}

int buffer_setmode(buffer_t *buf, enum buf_mode m)
{
	if(!m || (m & (m - 1))){
		return -1;
	}else{
		if(buf->ui_mode & UI_INSERT_ANY)
			buf->prev_insert = *buf->ui_pos;

		if(m & UI_VISUAL_ANY){
			if((buf->ui_mode & UI_VISUAL_ANY) == 0){
				/* from non-visual to visual */
				*buffer_uipos_alt(buf) = *buf->ui_pos;
			}
		}else{
			/* from a visual, save state */
			buf->prev_visual.mode = buf->ui_mode;

			buf->prev_visual.npos = *buf->ui_pos;
			buf->prev_visual.vpos = *buffer_uipos_alt(buf);
		}

		buf->ui_mode = m;
		return 0;
	}
}

static
buffer_t *buffer_new_file(FILE *f)
{
	/* TODO: mmap() */
	buffer_t *b = buffer_new();
	buffer_replace_file(b, f);

	return b;
}

void buffer_new_fname(buffer_t **pb, const char *fname, int *err)
{
	buffer_t *b;
	FILE *f;

	f = fopen(fname, "r");
	if(!f){
got_err:
		*err = 1;
		b = buffer_new();
		b->modified = true; /* editing a non-existant file, etc */
		goto fin;
	}

	b = buffer_new_file(f);
	fclose(f);

	if(!b)
		goto got_err;

	*err = 0;

fin:
	buffer_set_fname(b, fname);
	*pb = b;
}

int buffer_replace_file(buffer_t *b, FILE *f)
{
	list_t *l = list_new_file(f, &b->eol);

	if(!l)
		return 0;

	list_free(b->head);
	b->head = l;

	b->mtime = time(NULL);

	return 1;
}

int buffer_replace_fname(buffer_t *b, const char *fname)
{
	FILE *f = fopen(fname, "r");
	int r;

	if(!f)
		return 0;

	r = buffer_replace_file(b, f);
	fclose(f);

	if(r)
		b->ui_pos->y = MIN(b->ui_pos->y, list_count(b->head));

	return r;
}

int buffer_write_file(buffer_t *b, int n, FILE *f, bool eol)
{
	int r = list_write_file(b->head, n, f, eol);
	b->mtime = time(NULL);
	b->modified = false;
	return r;
}

void buffer_set_fname(buffer_t *b, const char *s)
{
	if(b->fname != s){
		free(b->fname);
		b->fname = ustrdup(s);
	}
}

const char *buffer_fname(const buffer_t *b)
{
	return b->fname;
}

void buffer_inschar_at(buffer_t *buf, char ch, int *x, int *y)
{
	switch(ch){
		case CTRL_AND('?'):
		case CTRL_AND('H'):
		case 127:
			if(*x > 0)
				buffer_delchar(buf, x, y);
			break;

		default:
			list_inschar(buf->head, x, y, ch);
			break;
	}
	buf->modified = true;
}

static void buffer_replacechar_at(buffer_t *buf, char ch, int *x, int *y)
{
	region_t r = { .type = REGION_COL };
	r.begin = r.end = (point_t){ .y = *y, .x = *x };

	/* TODO: replace column */

	list_iter_region(buf->head, &r, LIST_ITER_EVAL_NL, replace_iter, &ch);
}

static void buffer_inscolchar(buffer_t *buf, char ch, unsigned ncols)
{
	for(int i = ncols - 1; i >= 0; i--){
		int y = buf->ui_pos->y + i;
		int x = buf->ui_pos->x;
		int *px = &x;
		int *py = &y;

		/* update x and y in the last case */
		if(i == 0){
			px = &buf->ui_pos->x;
			py = &buf->ui_pos->y;
		}

		(buf->ui_mode & UI_REPLACE
		 ? buffer_replacechar_at
		 : buffer_inschar_at)(buf, ch, px, py);
	}
}

void buffer_inschar(buffer_t *buf, char ch)
{
	if(buf->ui_mode != UI_INSERT_COL || isnewline(ch)){
		/* can't col-insert a newline, revert */
		if(buf->ui_mode == UI_INSERT_COL)
			buffer_setmode(buf, UI_INSERT);

		buffer_inscolchar(buf, ch, 1);
	}else{
		buffer_inscolchar(buf, ch, buf->col_insert_height);
	}
}

void buffer_delchar(buffer_t *buf, int *x, int *y)
{
	list_delchar(buf->head, x, y);
	buf->modified = true;
}

static
void buffer_delregion_f(buffer_t *buf, const region_t *region, point_t *out)
{
	list_t *del = list_delregion(&buf->head, region);

	if(del){
		release(yank_push(yank_new(del, region->type)), yank_free);

		buf->modified = true;
	}
}
struct buffer_action buffer_delregion = {
	.fn = buffer_delregion_f
};

static
void buffer_yankregion_f(buffer_t *buf, const region_t *region, point_t *out)
{
	list_t *yanked = list_delregion(&buf->head, region);

	if(yanked){
		yank *yank = yank_new(yanked, region->type);
		yank_push(yank);

		buffer_insyank(buf, yank, /*prepend:*/true, /*modify:*/false);
		release(yank, yank_free);
	}

	/* restore ui pos (set in buffer_insyank) */
	*out = region->begin;
}
struct buffer_action buffer_yankregion = {
	.fn = buffer_yankregion_f
};

void buffer_insyank(buffer_t *buf, const yank *y, bool prepend, bool modify)
{
	yank_put_in_list(y,
			list_seekp(&buf->head, buf->ui_pos->y, true),
			prepend,
			&buf->ui_pos->y, &buf->ui_pos->x);

	if(modify)
		buf->modified = true;
}

static
void buffer_joinregion_f(buffer_t *buf, const region_t *region, point_t *out)
{
	/* could use 'how' here - columns and lines only make sense */
	list_t *l = list_seek(buf->head, region->begin.y, 0);
	const int mid = l ? l->len_line : 0;

	list_joinregion(&buf->head, region);

	if(l)
		out->x = mid;
	buf->modified = true;
}
struct buffer_action buffer_joinregion = {
	.fn = buffer_joinregion_f,
	.always_linewise = true
};

static
void buffer_indent2(
		buffer_t *buf, const region_t *region,
		point_t *out, int dir)
{
	/* region is sorted by y */
	const int min_x = MIN(region->begin.x, region->end.x);
	list_t *pos = dir < 0
		? list_seek(buf->head, buf->ui_pos->y, 0)
		: NULL;

	for(int y = region->begin.y; y <= region->end.y; y++){
		int x = 0;

		switch(region->type){
			case REGION_CHAR:
			case REGION_LINE:
				break;
			case REGION_COL:
				/* indent from the first col onwards */
				x = min_x;
		}

		if(dir > 0){
			buffer_inschar_at(buf, '\t', &x, &y);
		}else{
			if(pos){
				if(pos->len_line > 0 && pos->line[0] == '\t')
					memmove(pos->line, pos->line + 1, --pos->len_line);

				pos = pos->next;
			}else{
				break;
			}
		}
	}
	buf->modified = true;
}

int buffer_filter(
		buffer_t *buf, const region_t *reg,
		const char *cmd)
{
	buf->modified = true;
	return list_filter(&buf->head, reg, cmd);
}

static
void buffer_indent_f(buffer_t *buf, const region_t *region, point_t *out)
{
	buffer_indent2(buf, region, out, 1);
	buf->modified = true;
}
struct buffer_action buffer_indent = {
	.fn = buffer_indent_f,
	.always_linewise = true
};

static
void buffer_unindent_f(buffer_t *buf, const region_t *region, point_t *out)
{
	buffer_indent2(buf, region, out, -1);
	buf->modified = true;
}
struct buffer_action buffer_unindent = {
	.fn = buffer_unindent_f,
	.always_linewise = true
};

static int ctoggle(int c)
{
	return islower(c) ? toupper(c) : tolower(c);
}

static bool buffer_case_cb(char *s, list_t *l, int y, void *ctx)
{
	*s = (*(int (**)(int))ctx)(*s);

	return true;
}

void buffer_caseregion(
		buffer_t *buf,
		enum case_tog tog_type,
		const region_t *r)
{
	int (*f)(int) = NULL;

	switch(tog_type){
		case CASE_TOGGLE: f = ctoggle; break;
		case CASE_UPPER:  f = toupper; break;
		case CASE_LOWER:  f = tolower; break;
	}
	assert(f);

	list_iter_region(buf->head, r, 0, buffer_case_cb, &f);
	buf->modified = true;
}

void buffer_insline(buffer_t *buf, int dir)
{
	list_insline(&buf->head, &buf->ui_pos->x, &buf->ui_pos->y, dir);
	buf->modified = true;
}

buffer_t *buffer_topleftmost(buffer_t *b)
{
	for(;;){
		int changed = 0;
		for(; b->neighbours[BUF_LEFT]; changed = 1, b = b->neighbours[BUF_LEFT]);
		for(; b->neighbours[BUF_UP];   changed = 1, b = b->neighbours[BUF_UP]);
		if(!changed)
			break;
	}

	return b;
}

void buffer_add_neighbour(buffer_t *to, enum buffer_neighbour loc, buffer_t *new)
{
	/* TODO; leaks, etc */
	//buffer_t *sav = to->neighbours[loc];
	enum buffer_neighbour rloc;

#define OPPOSITE(a, b) case a: rloc = b; break

	switch(loc){
		OPPOSITE(BUF_LEFT,  BUF_RIGHT);
		OPPOSITE(BUF_RIGHT, BUF_LEFT);
		OPPOSITE(BUF_DOWN,  BUF_UP);
		OPPOSITE(BUF_UP,    BUF_DOWN);
	}

	to->neighbours[loc] = new;
	new->neighbours[rloc] = to;
}

list_t *buffer_current_line(const buffer_t *b, bool create)
{
	return list_seek(b->head, b->ui_pos->y, create);
}

unsigned buffer_nlines(const buffer_t *b)
{
	return list_count(b->head);
}

static char *buffer_find2(
		char *haystack, size_t haystack_sz,
		const char *needle,
		unsigned off, int dir)
{
	return dir < 0
		? tim_strrevstr(haystack, off, needle)
		: tim_strstr(haystack + off, haystack_sz - off, needle);
}

bool buffer_findat(const buffer_t *buf, const char *search, point_t *at, int dir)
{
	list_t *l = list_seek(buf->head, at->y, 0);

	if(!l){
		if(dir < 0){
			l = list_last(buf->head, &at->y);
		}else{
			return false;
		}
	}

	/* search at the next char */
	l = list_advance_x(l, dir, &at->y, &at->x);

	while(l){
		char *p;
		if((unsigned)at->x < l->len_line
		&& (p = buffer_find2(l->line, l->len_line, search, at->x, dir)))
		{
			at->x = p - l->line;
			at->y = at->y;
			return true;
		}

		l = list_advance_y(l, dir, &at->y, &at->x);
	}

	return false;
}

point_t buffer_toscreen(const buffer_t *buf, point_t const *pt)
{
	list_t *l = list_seek(buf->head, buf->ui_pos->y, 0);
	int xoff = 0;

	if(l && l->len_line > 0)
		for(int x = MIN((unsigned)buf->ui_pos->x, l->len_line - 1);
				x >= 0;
				x--)
			xoff += nc_charlen(l->line[x]) - 1;

	return (point_t){
		buf->screen_coord.x + pt->x - buf->ui_start.x + xoff,
		buf->screen_coord.y + pt->y - buf->ui_start.y
	};
}

point_t *buffer_uipos_alt(buffer_t *buf)
{
	if(buf->ui_pos == &buf->ui_vpos)
		return &buf->ui_npos;
	return &buf->ui_vpos;
}
