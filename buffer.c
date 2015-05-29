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
#include "buffers.h"
#include "indent.h"

void buffer_free(buffer_t *b)
{
	list_free(b->head);
	free(b->fname);
	free(b);
}

buffer_t *buffer_new()
{
	buffer_t *b = umalloc(sizeof *b);

	retain_init(b);

	b->head = list_new(NULL);
	b->eol = true; /* default to nice eol */
	return b;
}

static void buffer_replace_list(buffer_t *b, list_t *l)
{
	list_free(b->head);
	b->head = l;
}

static int buffer_replace_file(buffer_t *b, FILE *f)
{
	list_t *l = list_new_file(f, &b->eol);

	if(!l)
		return 0;

	buffer_replace_list(b, l);

	b->mtime = time(NULL);

	return 1;
}

void buffer_replace_fname(
		buffer_t *const b, const char *fname,
		const char **const err)
{
	*err = NULL;

	FILE *f = fopen(fname, "r");
	if(!f){
err:
		*err = strerror(errno);
		return;
	}

	if(!buffer_replace_file(b, f)){
		fclose(f);
		goto err;
	}
	fclose(f);

	buffer_set_fname(b, fname);
}

buffer_t *buffer_new_file_nofind(FILE *f)
{
	buffer_t *b = buffer_new();

	if(buffer_replace_file(b, f))
		return b;

	buffer_free(b);

	return NULL;
}

void buffer_new_fname(
		buffer_t **pb, const char *fname,
		const char **const err)
{
	*err = NULL;

	/* look for an existing buffer first */
	buffer_t *b = buffers_find(fname);

	if(b){
		*pb = retain(b);
		return;
	}

	FILE *f = fopen(fname, "r");
	if(!f){
got_err:
		*err = strerror(errno);
		b = buffer_new();
		b->modified = false; /* editing a non-existant file, etc */
		goto fin;
	}

	b = buffer_new_file_nofind(f);
	fclose(f);

	if(!b){
		if(errno == EISDIR){
			DIR *d = opendir(fname);
			if(!d)
				goto got_err;

			list_t *l = list_from_dir(d);

			const int save_errno = errno;
			closedir(d);
			errno = save_errno;

			if(!l)
				goto got_err;

			b = buffer_new();
			buffer_replace_list(b, l);
		}else{
			goto got_err;
		}
	}

fin:
	buffer_set_fname(b, fname);
	*pb = b;
}

int buffer_opencount(const buffer_t *b)
{
	return b->retains.rcount;
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

static list_t *buffer_last_indent_line(buffer_t *buf, int y)
{
	list_t *l = list_seek(buf->head, y, false);

	if(!l)
		return NULL;

	/* scan backwards until we hit a non-empty line */
	while(l->prev){
		l = l->prev;
		if(!isallspace(l->line, l->len_line))
			return l;
	}

	return NULL;
}

void buffer_smartindent(buffer_t *buf, int *const x, int y)
{
	list_t *l = buffer_last_indent_line(buf, y);
	if(!l)
		return;

	int indent = indent_count(l, true);

	list_t *current = list_seek(buf->head, y, false);
	if(current)
		indent_adjust(current, *x, &indent);

	if(indent < 0)
		indent = 0;
	*x = indent;
}

static void buffer_unindent_empty(buffer_t *buf, int *const x, int y)
{
	if(y == 0)
		return;

	list_t *l = buffer_last_indent_line(buf, y);
	if(!l)
		return;
	int indent = indent_count(l, true);

	if(indent == 0)
		return;

	*x = indent - 1;
}

void buffer_inschar_at(buffer_t *buf, char ch, int *x, int *y)
{
	bool indent = false;
	bool inschar = true;

	switch(ch){
		case CTRL_AND('?'):
		case CTRL_AND('H'):
		case 127:
			if(*x > 0)
				buffer_delchar(buf, x, y);
			inschar = false;
			break;

		case '}':
		{
			list_t *l = list_seek(buf->head, *y, false);

			/* if we've just started a new line,
			 * or it's all space before us, this unindents
			 */
			if(!l || !l->line || isallspace(l->line, *x)){
				buffer_unindent_empty(buf, x, *y);
				indent = false;
			}
			break;
		}

		case '\n':
			indent = true;
	}

	if(inschar){
		list_inschar(buf->head, x, y, ch, /*autogap*/0);
		if(indent)
			buffer_smartindent(buf, x, *y);
	}

	buf->modified = true;
}

void buffer_inscolchar(
		buffer_t *buf, char ch, unsigned ncols,
		point_t *const ui_pos)
{
	for(int i = ncols - 1; i >= 0; i--){
		int y = ui_pos->y + i;
		int x = ui_pos->x;
		int *px = &x;
		int *py = &y;

		/* update x and y in the last case */
		if(i == 0){
			px = &ui_pos->x;
			py = &ui_pos->y;
		}

		buffer_inschar_at(buf, ch, px, py);
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

		point_t ui_pos = region->begin;

		buffer_insyank(buf, yank, &ui_pos,
				/*prepend:*/true, /*modify:*/false);

		release(yank, yank_free);
	}

	/* restore ui pos (set in buffer_insyank) */
	*out = region->begin;
}
struct buffer_action buffer_yankregion = {
	.fn = buffer_yankregion_f
};

void buffer_insyank(
		buffer_t *buf, const yank *y,
		point_t *ui_pos,
		bool prepend, bool modify)
{
	yank_put_in_list(y,
			&buf->head,
			prepend,
			&ui_pos->y,
			&ui_pos->x);

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
		? list_seek(buf->head, region->begin.y, 0)
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

void buffer_insline(buffer_t *buf, int dir, point_t *ui_pos)
{
	list_insline(&buf->head, &ui_pos->x, &ui_pos->y, dir);
	buf->modified = true;
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
