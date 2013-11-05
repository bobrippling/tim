#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "mem.h"
#include "str.h"
#include "mem.h"

list_t *list_new(list_t *prev)
{
	list_t *l = umalloc(sizeof *l);
	l->prev = prev;
	return l;
}

list_t *list_copy_deep(const list_t *const l, list_t *prev)
{
	if(!l)
		return NULL;
	list_t *new = list_new(prev);
	new->len_line = l->len_line;
	new->len_malloc = l->len_line; /* len_line, not len_malloc */
	new->line = ustrdup_len(l->line, l->len_line);
	new->next = list_copy_deep(l->next, new);
	return new;
}

static void mmap_new_line(list_t **pcur, char *p, char **panchor)
{
	char *data = umalloc(p - *panchor + 1);

	memcpy(data, *panchor, p - *panchor);

	list_t *cur = *pcur;
	cur->line = data;
	cur->next = list_new(cur);
	*pcur = cur->next;

	*panchor = p + 1;
}

static list_t *mmap_to_lines(char *mem, size_t len, bool *eol)
{
	list_t *head = list_new(NULL), *cur = head;
	char *const last = mem + len;

	char *p, *begin;
	for(p = begin = mem; p < last; p++)
		if(*p == '\n')
			mmap_new_line(&cur, p, &begin);

	int last_is_nl = (p == begin);
	*eol = last_is_nl;
	if(!last_is_nl)
		mmap_new_line(&cur, p, &begin);

	return head;
}

static list_t *list_new_fd_read(int fd, bool *eol)
{
	list_t *l = list_new(NULL);

	int y = 0, x = 0;
	bool nl = false;
	int ch;
	int r;
	while((r = read(fd, &ch, 1)) == 1){
		if(nl){
			list_inschar(l, &x, &y, '\n');
			nl = false;
		}

		if(ch == '\n')
			nl = true;
		else
			list_inschar(l, &x, &y, ch);
	}

	*eol = nl;

	return l;
}

static list_t *list_new_fd(int fd, bool *eol)
{
	struct stat st;
	if(fstat(fd, &st) == -1)
		return NULL;

	if(st.st_size == 0)
		goto fallback; /* could be stdin */

	void *mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	if(mem == MAP_FAILED){
		if(errno == EINVAL)
fallback:
			return list_new_fd_read(fd, eol);

		return NULL;
	}

	list_t *l = mmap_to_lines(mem, st.st_size, eol);
	munmap(mem, st.st_size);

	return l;

}

list_t *list_new_file(FILE *f, bool *eol)
{
#ifdef FAST_LIST_INIT
	char buf[256];
	size_t n;
	list_t *l, *head;

	l = head = list_new();

	while((n = fread(buf, sizeof *buf, sizeof buf, f)) > 0){
		char *nl = memchr(buf, '\n', n);
		if(!nl)
			abort(); /* TODO: */

		l->len_malloc = l->len_line = nl - buf;

		if(l->len_malloc){
			l->line = umalloc(len);

		}
	}

	if(n < 0){
		list_free(l);
		return NULL;
	}

	return l;
#else
	return list_new_fd_read(fileno(f), eol);
#endif
}

static int list_write_fd(list_t *l, int n, int fd, bool eol)
{
	for(; l && (n == -1 || n --> 0); l = l->next){
		if(write(fd, l->line, l->len_line) != (int)l->len_line)
			goto got_err;

		if(l->next || eol)
			if(write(fd, "\n", 1) != 1)
				goto got_err;
	}

	return 0;
got_err:
	return -1;
}

int list_write_file(list_t *l, int n, FILE *f, bool eol)
{
	return list_write_fd(l, n, fileno(f), eol);
}

void list_free(list_t *l)
{
	if(l){
		list_t *next = l->next;

		free(l->line);
		free(l);

		/* tco */
		list_free(next);
	}
}

list_t **list_seekp(list_t **pl, int y, bool creat)
{
	if(!*pl){
		if(creat)
			*pl = list_new(NULL);
		else
			return NULL;
	}

	while(y > 0){
		if(!(*pl)->next){
			if(creat)
				(*pl)->next = list_new(*pl);
			else
				return NULL;
		}

		pl = &(*pl)->next;
		y--;
	}

	return pl;
}

list_t *list_seek(list_t *l, int y, bool creat)
{
	list_t **p = list_seekp(&l, y, creat);
	return p ? *p : NULL;
}

static int list_evalnewlines(list_t *l)
{
	if(l->len_line == 0)
		return 0;

	int r = 0;
	for(size_t i = l->len_line - 1; ; i--){
		char ch = l->line[i];

		if(ch == '\n' /* XXX: not '\r' / ^M */){
			int cut_len = l->len_line - i;
			char *cut;
			if(cut_len > 0){
				cut = umalloc(cut_len);
				/* cut_len-1, don't copy the \n */
				memcpy(cut, l->line + i + 1, cut_len-1);
			}else{
				cut = NULL;
				cut_len = 0;
			}

			l->len_line = i;

			/* insert a line after */
			list_insline(&l, &(int){0}, &(int){0}, 1);

			list_t *nl = l->next;
			nl->line = cut;
			nl->len_malloc = cut_len;
			nl->len_line = cut_len - 1;

			r = 1; /* we replaced */
		}

		if(i == 0)
			break;
	}

	return r;
}

void list_inschar(list_t *l, int *x, int *y, char ch)
{
	l = list_seek(l, *y, true);

	if((unsigned)*x >= l->len_malloc){
		const int old_len = l->len_malloc;

		l->len_malloc = *x + 1;
		l->line = urealloc(l->line, l->len_malloc);

		memset(l->line + old_len, 0, l->len_malloc - old_len);
	}else{
		/* shift stuff up */
		l->line = urealloc(l->line, ++l->len_malloc);
		memmove(l->line + *x + 1, l->line + *x, l->len_malloc - *x - 1);
	}

	for(int i = l->len_line; i < *x; i++){
		l->line[i] = ' ';
		l->len_line++;
	}

	l->line[*x] = ch;
	l->len_line++;
	if(list_evalnewlines(l)){
		*x = 0;
		++*y;
	}else{
		++*x;
	}
}

void list_delchar(list_t *l, int *x, int *y)
{
	l = list_seek(l, *y, false);

	--*x;

	if(!l)
		return;

	if((unsigned)*x >= l->len_line)
		return;

	memmove(l->line + *x, l->line + *x + 1, --l->len_line - *x);
}

static
list_t *list_dellines(list_t **pl, list_t *prev, unsigned n)
{
	if(n == 0)
		return NULL;

	list_t *l = *pl;

	if(!l)
		return NULL;

	if(n == 1){
		list_t *adv = l->next;
		l->next = NULL;

		*pl = adv;

	}else{
		list_t *end_m1 = list_seek(l, n - 2, false);

		if(end_m1){
			*pl = end_m1->next;
			end_m1->next = NULL;
		}else{
			/* delete everything */
			*pl = NULL;
		}
	}

	if(*pl)
		(*pl)->prev = prev;

	return l;
}

list_t *list_tail(list_t *l)
{
	for(; l->next; l = l->next);
	return l;
}

list_t *list_append(list_t *accum, list_t *new)
{
	if(!accum)
		return new;

	list_t *last = list_tail(accum);

	last->next = new;
	new->prev = last;

	return accum;
}

list_t *list_delregion(list_t **pl, const region_t *region)
{
	assert(region->begin.y <= region->end.y);
	assert(region->begin.y < region->end.y || region->begin.x < region->end.x);

	list_t **seeked = list_seekp(pl, region->begin.y, false);

	if(!seeked || !*seeked)
		return NULL;

	list_t *deleted = NULL;

	switch(region->type){
		case REGION_LINE:
			deleted = list_dellines(
					seeked, (*seeked)->prev,
					region->end.y - region->begin.y + 1);
			break;
		case REGION_CHAR:
		{
			list_t *l = *seeked;
			size_t line_change = region->end.y - region->begin.y;

			if(line_change > 1)
				deleted = list_append(
						deleted,
						list_dellines(&l->next, l, line_change));

			if(line_change > 0){
				/* join the lines */
				list_t *next = l->next;
				size_t nextlen;
				if((unsigned)region->end.x >= next->len_line){
					nextlen = 0;
				}else{
					nextlen = next->len_line - region->end.x;
				}
				size_t fulllen = region->begin.x + nextlen;

				if(l->len_malloc < fulllen)
					l->line = urealloc(l->line, l->len_malloc = fulllen);

				memcpy(l->line + region->begin.x,
						next->line + region->end.x,
						nextlen);
				l->len_line = fulllen;

				deleted = list_append(
						deleted,
						list_dellines(&l->next, l, 1));

			}else{
				if(!l->len_line || (unsigned)region->end.x > l->len_line)
					return NULL;

				size_t diff = region->end.x - region->begin.x;

				list_t *part = list_new(NULL);
				part->line = umalloc(diff + 1);
				part->len_line = diff;
				part->len_malloc = diff + 1;
				memcpy(part->line, l->line + region->begin.x, diff);

				memmove(
						l->line + region->begin.x,
						l->line + region->end.x,
						l->len_line - region->begin.x - 1);

				l->len_line -= diff;

				deleted = list_append(deleted, part);
			}
			break;
		}
		case REGION_COL:
		{
			list_t *pos = *seeked;
			point_t begin = region->begin, end = region->end;

			/* should've been sorted also */
			assert(begin.x <= end.x);

			for(int i = begin.y;
					i <= end.y && pos;
					i++, pos = pos->next)
			{
				if((unsigned)begin.x < pos->len_line){
					struct
					{
						char *str;
						size_t len;
					} removed;

					if((unsigned)end.x >= pos->len_line){
						/* delete all */
						removed.len = pos->len_line - begin.x + 1;
						removed.str = ustrdup_len(
								pos->line + begin.x,
								removed.len);

						pos->len_line = begin.x;

					}else{
						char *str = pos->line;

						removed.len = end.x - begin.x;
						removed.str = ustrdup_len(
								str + begin.x,
								removed.len);

						memmove(
								str + begin.x,
								str + end.x,
								pos->len_line - end.x);

						pos->len_line -= end.x - begin.x;
					}

					/* removed text */
					list_t *new = list_new(NULL);
					new->line = removed.str;
					new->len_malloc = new->len_line = removed.len;
					deleted = list_append(deleted, new);
				}
			}
			break;
		}
	}

	return deleted;
}

void list_joinregion(list_t **pl, const region_t *region)
{
	if(region->begin.y == region->end.y)
		return;

	assert(region->begin.y < region->end.y);

	list_t *l, *start = list_seek(*pl, region->begin.y, false);
	int i;

	if(!start)
		return;

	if(start->line)
		str_rtrim(start->line, &start->len_line);

	for(l = start->next, i = region->begin.y + 1;
			l && i < region->end.y;
			l = l->next, i++)
	{
		if(!l->line)
			continue;

		str_ltrim(l->line, &l->len_line);

		/* + 1 for \0, +1 for ' ' */
		const size_t target = start->len_line + l->len_line + 2;

		if(start->len_malloc < target)
			start->len_malloc = target;

		start->line = urealloc(
				start->line,
				start->len_malloc);

		start->line[start->len_line++] = ' ';

		memcpy(start->line + start->len_line,
				l->line,
				l->len_line);

		start->len_line += l->len_line;
	}

	if(i > region->begin.y)
		list_dellines(&start->next, start, i - region->begin.y);
}

void list_insline(list_t **pl, int *x, int *y, int dir)
{
	list_t *l, *save;

	*x = 0;

	if(dir < 0 && *y == 0){
		/* special case */
		l = *pl;
		(*pl = list_new(NULL))->next = l;
		if(l)
			l->prev = *pl;
		return;
	}

	if(dir < 0)
		--*y;

	l = list_seek(*pl, *y, true);

	save = l->next;
	l->next = list_new(l);
	l->next->next = save;
	if(save)
		save->prev = l->next;

	++*y;
}

void list_replace_at(list_t *l, int *px, int *py, char *with)
{
	l = list_seek(l, *py, true);

	const int with_len = strlen(with);
	int x = *px;

	if((unsigned)(x + with_len) >= l->len_malloc){
		size_t new_len = x + with_len + 1;
		l->line = urealloc(l->line, new_len);
		memset(l->line + l->len_malloc, ' ', new_len - l->len_malloc);
		l->len_line = l->len_malloc = new_len;
	}

	char *p = l->line + x;
	memcpy(p, with, with_len);

	/* convert r\n to newlines */
	if(list_evalnewlines(l)){
		*px = 0;
		++*py;
	}else{
		*px += with_len - 1;
	}
}

int list_count(list_t *l)
{
	int i;
	for(i = 0; l->next; l = l->next, i++);
	return i;
}

int list_filter(
		list_t **pl, const region_t *region,
		const char *cmd)
{
	pl = list_seekp(pl, region->begin.y, false);

	if(!pl)
		return -1;

	int child_in[2], child_out[2];
	if(pipe(child_in))
		return -1;
	if(pipe(child_out)){
		const int e = errno;
		close(child_in[0]);
		close(child_in[1]);
		errno = e;
		return -1;
	}

	switch(fork()){
		case -1:
		{
			const int e = errno;
			close(child_out[0]);
			close(child_out[1]);
			errno = e;
			return -1;
		}
		case 0:
			dup2(child_in[0], 0);
			dup2(child_out[1], 1);
			for(int i = 0; i < 2; i++)
				close(child_in[i]), close(child_out[i]);

			execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
			exit(127);
	}

	/* parent */
	close(child_in[0]), close(child_out[1]);

	const unsigned region_height = region->end.y - region->begin.y;

	list_t **phead = pl;
	list_t *tail = list_seek(*phead, region_height, false);

	/* write our lines to child_in */
	list_write_fd(
			*pl, region_height,
			child_in[1], /*eol:*/true);

	/* writes done */
	close(child_in[1]);

	/* read from child_out */
	bool eol;
	list_t *l_read = list_new_fd(child_out[0], &eol);
	close(child_out[0]);

	list_t *const gone = *pl;

	*pl = l_read;
	for(; l_read->next; l_read = l_read->next);
	l_read->next = tail;
	tail->prev = l_read;

	list_t *gone_tail;
	for(gone_tail = gone;
			gone_tail && gone_tail->next != tail;
			gone_tail = gone_tail->next);

	if(gone_tail){
		gone_tail->next = NULL;
		list_free(gone);
	}/* else we've lost `gone'?? */

	return 0;
}

list_t *list_advance_y(
		list_t *l, const int dir, int *py, int *px)
{
	*py += dir;

	if(dir > 0){
		if(px)
			*px = 0;
		return l->next;
	}else{
		l = l->prev;
		if(px && l)
			*px = l->len_line > 0 ? l->len_line - 1 : 0;
		return l;
	}
}

list_t *list_advance_x(
		list_t *l, const int dir, int *py, int *px)
{
	if(dir > 0){
		if((unsigned)++*px >= l->len_line){
			l = l->next;
			++*py;
			*px = 0;
		}
	}else{
		if(*px == 0)
			l = list_advance_y(l, dir, py, px);
		else
			--*px;
	}
	return l;
}
