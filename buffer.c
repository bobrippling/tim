#include <stdio.h>
#include <stdlib.h>
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

#define TODO() fprintf(stderr, "TODO! %s\n", __func__)

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
	return b;
}

void buffer_togglev(buffer_t *buf)
{
	buf->ui_pos = (buf->ui_pos == &buf->ui_npos)
		? &buf->ui_vpos
		: &buf->ui_npos;
}

int buffer_setmode(buffer_t *buf, enum buf_mode m)
{
	if(!m || (m & (m - 1))){
		return -1;
	}else{
		if((buf->ui_mode & UI_VISUAL_ANY) == 0
		&& m & UI_VISUAL_ANY)
		{
			/* from non-visual to visual */
			*buffer_uipos_alt(buf) = *buf->ui_pos;
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

	return r;
}

int buffer_write_file(buffer_t *b, int n, FILE *f, bool eol)
{
	return list_write_file(b->head, n, f, eol);
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

void buffer_inschar(buffer_t *buf, int *x, int *y, char ch)
{
	list_inschar(&buf->head, x, y, ch);
}

void buffer_delchar(buffer_t *buf, int *x, int *y)
{
	list_delchar(buf->head, x, y);
}

static
void buffer_delregion_f(buffer_t *buf, const region_t *region, point_t *out)
{
	list_t *del = list_delregion(&buf->head, region);

	yank_push(del);
}
struct buffer_action buffer_delregion = { .fn = buffer_delregion_f };

void buffer_inslist(buffer_t *buf, list_t *l)
{
	list_t *bl = buffer_current_line(buf);

	list_t *tail = list_tail(l);

	tail->next = bl->next;
	tail->next->prev = tail;

	bl->next = l;
	l->prev = bl;
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
}
struct buffer_action buffer_joinregion = { .fn = buffer_joinregion_f, .is_linewise = 1 };

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

	for(int y = region->begin.y; y < region->end.y; y++){
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
			buffer_inschar(buf, &x, &y, '\t');
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
}

int buffer_filter(
		buffer_t *buf, const region_t *reg,
		const char *cmd)
{
	return list_filter(&buf->head, reg, cmd);
}

static
void buffer_indent_f(buffer_t *buf, const region_t *region, point_t *out)
{
	buffer_indent2(buf, region, out, 1);
}
struct buffer_action buffer_indent = { .fn = buffer_indent_f,  .is_linewise = 1 };

static
void buffer_unindent_f(buffer_t *buf, const region_t *region, point_t *out)
{
	buffer_indent2(buf, region, out, -1);
}
struct buffer_action buffer_unindent = { .fn = buffer_unindent_f,  .is_linewise = 1 };

void buffer_replace_chars(buffer_t *buf, int ch, unsigned n)
{
	char *with = umalloc(n + 1);

	memset(with, ch, n);
	with[n] = '\0';

	list_replace_at(buf->head,
			&buf->ui_pos->x, &buf->ui_pos->y,
			with);

	free(with);
}

static int ctoggle(int c)
{
	return islower(c) ? toupper(c) : tolower(c);
}

void buffer_case(buffer_t *buf, enum case_tog tog_type, unsigned n)
{
	list_t *l = list_seek(buf->head, buf->ui_pos->y, 0);

	int (*f)(int) = NULL;

	switch(tog_type){
		case CASE_TOGGLE: f = ctoggle; break;
		case CASE_UPPER:  f = toupper; break;
		case CASE_LOWER:  f = tolower; break;
	}
	assert(f);

	unsigned i;
	for(i = buf->ui_pos->x; n > 0; n--, i++){
		if(i >= l->len_line)
			break;

		char *p = &l->line[i];
		*p = f(*p);
	}

	buf->ui_pos->x = i;
}

void buffer_insline(buffer_t *buf, int dir)
{
	list_insline(&buf->head, &buf->ui_pos->x, &buf->ui_pos->y, dir);
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

list_t *buffer_current_line(const buffer_t *b)
{
	return list_seek(b->head, b->ui_pos->y, 0);
}

unsigned buffer_nlines(const buffer_t *b)
{
	return list_count(b->head);
}

const char *buffer_shortfname(const char *s)
{
#define SHORT_LEN_HALF 14
	static char buf[(SHORT_LEN_HALF + 2) * 2];
	size_t l = strlen(s);

	if(l > sizeof(buf)){
		const char *fin = s + l - SHORT_LEN_HALF;

		strncpy(buf, s, SHORT_LEN_HALF);
		buf[SHORT_LEN_HALF] = 0;

		snprintf(buf + SHORT_LEN_HALF - 1, sizeof(buf) - SHORT_LEN_HALF,
				"...%s", fin);

		return buf;
	}

	return s;
}

static char *strrevstr(char *haystack, unsigned off, const char *needle)
{
	const size_t nlen = strlen(needle);

	for(char *p = haystack + off;
			p >= haystack;
			p--)
	{
		if(!strncmp(p, needle, nlen))
			return p;
	}

	return NULL;
}

static char *buffer_find2(
		char *haystack, const char *needle,
		unsigned off, int dir)
{
	return dir < 0
		? strrevstr(haystack, off, needle)
		: strstr(haystack + off, needle);
}

bool buffer_findat(const buffer_t *buf, const char *search, point_t *at, int dir)
{
	list_t *l = list_seek(buf->head, at->y, 0);

	if(!l)
		return false;

	/* search at the next char */
	l = list_advance_x(l, dir, &at->y, &at->x);

	while(l){
		char *p;
		if((unsigned)at->x < l->len_line
		&& (p = buffer_find2(l->line, search, at->x, dir)))
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
	return (point_t){
		buf->screen_coord.x + pt->x - buf->ui_start.x,
		buf->screen_coord.y + pt->y - buf->ui_start.y
	};
}

point_t *buffer_uipos_alt(buffer_t *buf)
{
	if(buf->ui_pos == &buf->ui_vpos)
		return &buf->ui_npos;
	return &buf->ui_vpos;
}
