#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "pos.h"
#include "region.h"
#include "range.h"
#include "list.h"
#include "mem.h"
#include "str.h"
#include "mem.h"
#include "macros.h"
#include "word.h"

#define JOIN_CHAR ' '

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
	const size_t nchars = p - *panchor;
	char *data = umalloc(nchars + 1);

	memcpy(data, *panchor, nchars);
	data[nchars] = '\0';

	list_t *cur = *pcur;
	if(cur->line){
		cur->next = list_new(cur);
		cur = cur->next;
	}
	cur->line = data;
	cur->len_malloc = nchars + 1;
	cur->len_line = nchars;
	*pcur = cur;

	*panchor = p + 1;
}

static list_t *mmap_to_lines(char *mem, size_t len, bool *eol)
{
	list_t *head = list_new(NULL), *cur = head;
	char *const last = mem + len;

	char *p, *begin;
	for(p = begin = mem; p != last; p++)
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
	bool nl = false, empty = true;
	int ch = 0;
	int r;
	while((r = read(fd, &ch, 1)) == 1){
		if(nl){
			list_inschar(l, &x, &y, '\n', /*gap*/' ');
			nl = false;
		}

		if(ch == '\n')
			nl = true;
		else
			list_inschar(l, &x, &y, ch, /*gap*/' ');

		empty = false;
	}

	*eol = nl || empty;

	return l;
}

static list_t *list_new_fd(int fd, bool *eol)
{
	struct stat st;
	if(fstat(fd, &st) == -1)
		return NULL;

	if(st.st_size == 0)
		goto fallback; /* could be stdin */

	if(S_ISDIR(st.st_mode)){
		errno = EISDIR;
		return NULL;
	}

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
	return list_new_fd(fileno(f), eol);
#endif
}

list_t *list_from_dir(DIR *d)
{
	list_t *head = list_new(NULL), *prev = NULL;
	int dfd = dirfd(d);

	struct dirent *dent;
	while((errno = 0, dent = readdir(d))){
		size_t len = strlen(dent->d_name);
		struct stat st;
		const bool is_dir = (dfd != -1
				&& fstatat(dfd, dent->d_name, &st, 0) == 0
				&& S_ISDIR(st.st_mode));

		char *line;
		if(is_dir){
			len++;
			line = umalloc(len + 1);
			snprintf(line, len + 1, "%s/", dent->d_name);

		}else{
			line = ustrdup_len(dent->d_name, len);
		}

		list_t *l;

		if(!head->line){
			l = head;
		}else{
			assert(prev);
			l = list_new(prev);
			prev->next = l;
		}
		prev = l;

		l->line = line;
		l->len_line = len;
		l->len_malloc = len + 1;
	}

	if(errno){
		int save = errno;
		list_free(head);
		head = NULL;
		errno = save;
	}

	return head;
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
	while(l){
		list_t *next = l->next;

		free(l->line);
		free(l);

		l = next;
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

int list_evalnewlines1(list_t *l)
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

void list_inschar(
		list_t *l, int *x, int *y,
		char ch, char gapchar)
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

	if(!gapchar){
		/* autogap */
		for(int i = 0; i < *x && (unsigned)i < l->len_line; i++){
			if(!isspace(l->line[i])){
				gapchar = ' ';
				break;
			}
		}

		if(!gapchar){
			/* all space, go with a tab */
			gapchar = '\t';
		}
	}

	for(int i = l->len_line; i < *x; i++){
		l->line[i] = gapchar;
		l->len_line++;
	}

	l->line[*x] = ch;
	l->len_line++;
	if(list_evalnewlines1(l)){
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
	l->prev = NULL;

	list_t *end_m1 = list_seek(l, n - 1, false);

	if(end_m1){
		*pl = end_m1->next;
		end_m1->next = NULL;
	}else{
		/* delete everything */
		*pl = NULL;
	}

	if(*pl)
		(*pl)->prev = prev;
	if(l)
		l->prev = NULL;

	return l;
}

list_t *list_tail(list_t *l)
{
	for(; l && l->next; l = l->next);
	return l;
}

list_t *list_append(list_t *accum, list_t *at, list_t *new)
{
	if(!accum)
		return new;

	list_t *after = at->next;
	at->next = new;
	new->prev = at;

	if(after){
		list_t *newtail = list_tail(new);
		newtail->next = after;
		after->prev = newtail;
	}

	return accum;
}

list_t *list_delregion(list_t **pl, const region_t *region)
{
	if(region->begin.y > region->end.y)
		return NULL;

	if(region->type != REGION_LINE
	&& region->begin.y == region->end.y
	&& region->begin.x >= region->end.x)
	{
		return NULL;
	}

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

			/* create `deleted' */
			{
				deleted = list_new(NULL);

				if((unsigned)region->begin.x < l->len_line){
					char *this_line = l->line;
					size_t len = line_change == 0
							? (unsigned)region->end.x - region->begin.x
							: l->len_line - region->begin.x;

					char *pulled_out = ustrdup_len(
							this_line + region->begin.x,
							len);

					deleted->line = pulled_out;
					deleted->len_line = deleted->len_malloc = len;
				}
				/* else we leave deleted empty */
			}

			if(line_change > 1){
				deleted = list_append(
						deleted,
						list_tail(deleted),
						list_dellines(
							&l->next, l->prev,
							line_change - 1));
			}


			/* wipe lines */
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

				{
					list_t *pullout = list_new(NULL);
					deleted = list_append(deleted, list_tail(deleted), pullout);

					size_t len = MIN((unsigned)region->end.x, next->len_line);
					pullout->len_malloc = pullout->len_line = len;

					pullout->line = ustrdup_len(next->line, len);
				}

				memcpy(l->line + region->begin.x,
						next->line + region->end.x,
						nextlen);
				l->len_line = fulllen;

				list_free(list_dellines(&l->next, l->prev, 1));

			}else{
				if(!l->len_line || (unsigned)region->end.x > l->len_line)
					return deleted;

				size_t diff = region->end.x - region->begin.x;

				list_t *part = list_new(NULL);
				part->line = umalloc(diff + 1);
				part->len_line = diff;
				part->len_malloc = diff + 1;
				memcpy(part->line, l->line + region->begin.x, diff);

				memmove(
						l->line + region->begin.x,
						l->line + region->end.x,
						l->len_line - region->end.x);

				l->len_line -= diff;
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
					deleted = list_append(deleted, list_tail(deleted), new);
				}else{
					deleted = list_append(
							deleted, list_tail(deleted),
							list_new(NULL));
				}
			}
			break;
		}
	}

	return deleted;
}

void list_joinregion(list_t **pl, const region_t *region, const bool space)
{
	if(region->begin.y == region->end.y)
		return;

	assert(region->begin.y < region->end.y);

	list_t *l, *start = list_seek(*pl, region->begin.y, false);

	if(!start)
		return;

	if(start->line)
		str_rtrim(start->line, &start->len_line);

	size_t i;

	for(l = start->next, i = region->begin.y + 1;
			l && i <= (unsigned)region->end.y;
			l = l->next, i++)
	{
		if(!l->line)
			continue;

		str_ltrim(l->line, &l->len_line);

		/* + 1 for \0, +1 for JOIN_CHAR */
		const size_t target = start->len_line + l->len_line + 2;

		if(start->len_malloc < target)
			start->len_malloc = target;

		start->line = urealloc(
				start->line,
				start->len_malloc);

		if(space)
			start->line[start->len_line++] = JOIN_CHAR;

		memcpy(start->line + start->len_line,
				l->line,
				l->len_line);

		start->len_line += l->len_line;
	}

	size_t ndelete = i - (region->begin.y + 1);
	if(ndelete)
		list_dellines(&start->next, start, ndelete);
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

	/* list_seekp() to create if necessary,
	 * and make that creation available in *pl */
	l = *list_seekp(pl, *y, true);

	save = l->next;
	l->next = list_new(l);
	l->next->next = save;
	if(save)
		save->prev = l->next;

	++*y;
}

int list_count(const list_t *l)
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

	if(!pl){
		errno = EINVAL;
		return -1;
	}

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

	pid_t pid = fork();
	switch(pid){
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
			dup2(child_out[1], 2); /* capture stderr too */
			for(int i = 0; i < 2; i++)
				close(child_in[i]), close(child_out[i]);

			execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
			exit(127);
	}

	/* parent */
	close(child_in[0]), close(child_out[1]);

	const unsigned region_height = region->end.y - region->begin.y + 1;

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

	/* reap */
	(void)waitpid(pid, NULL, 0);

	list_t *const gone = *pl;

	*pl = l_read;
	l_read->prev = gone->prev;
	if(gone->prev)
		gone->prev->next = l_read;
	for(; l_read->next; l_read = l_read->next);
	l_read->next = tail;
	if(tail)
		tail->prev = l_read;

	list_t *gone_tail;
	for(gone_tail = gone;
			gone_tail && gone_tail->next != tail;
			gone_tail = gone_tail->next);

	if(gone_tail){
		gone_tail->next = NULL;
		list_free(gone);
	}/* else `gone' was at the end already */

	return 0;
}

static bool line_iter(
		list_t *l, size_t start, size_t end,
		size_t y,
		list_iter_f fn, void *ctx)
{
	for(size_t i = start; i < end && i < l->len_line; i++){
		bool r = fn(l->line + i, l, y, ctx);
		if(!r)
			return false;
	}
	return true;
}

void list_iter_region(
		list_t *l, const region_t *r,
		enum list_iter_flags flags,
		list_iter_f fn, void *ctx)
{
	size_t i = 0;
	size_t end = r->end.y - r->begin.y + 1;

	for(l = list_seek(l, r->begin.y, false);
			l && i < end; l = l->next, i++)
	{
		const size_t y = i + r->begin.y;
		bool next = true;

		switch(r->type){
			case REGION_LINE:
				if(flags & LIST_ITER_WHOLE_LINE)
					next = fn(l->line, l, y, ctx);
				else
					next = line_iter(l, 0, l->len_line, y, fn, ctx);
				break;
			case REGION_COL:
				next = line_iter(l, r->begin.x, r->end.x, y, fn, ctx);
				break;
			case REGION_CHAR:
				next = line_iter(l,
						i == 0 ? r->begin.x : 0,
						i+1 == end ? (unsigned)r->end.x : l->len_line,
						y, fn, ctx);
				break;
		}
		if(flags & LIST_ITER_EVAL_NL)
			list_evalnewlines1(l);
		if(!next)
			break;
	}
}

list_t *list_advance_y(
		list_t *l, const int dir, int *py, int *px)
{
	if(!l)
		return NULL;

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
	if(!l)
		return NULL;

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

list_t *list_last(list_t *l, int *py)
{
	*py = 0;
	if(!l)
		return NULL;
	for(; l->next; l = l->next, ++*py);
	return l;
}

char *list_word_at(list_t *l, point_t const *pt)
{
	l = list_seek(l, pt->y, false);
	if(!l)
		return NULL;

	const bool bigwords = false;

	/* forwards until we hit a word */
	point_t found = *pt;
	l = word_seek(l, +1, &found, W_KEYWORD, bigwords);
	if(!l || found.y != pt->y)
		return NULL;

	/* back until the start of the word */
	word_seek_to_end(l, -1, &found, bigwords);

	/* forwards until the end of the word */
	point_t end = found;
	word_seek_to_end(l, +1, &end, bigwords);

	return ustrdup_len(l->line + found.x, end.x - found.x + 1);
}

static bool isfnamechar(char c)
{
	switch(c){
		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '0' ... '9':
			return true;
	}

	const char *fname_chars = "/.-_+,#$%~=";
	return strchr(fname_chars, c);
}

char *list_fname_at(list_t *l, point_t const *pt)
{
	l = list_seek(l, pt->y, false);
	if(!l)
		return NULL;

	if((unsigned)pt->x >= l->len_line)
		return NULL;

	int start = pt->x;
	if(isfnamechar(l->line[start])){
		/* look backwards for the start */
		for(; start >= 0 && isfnamechar(l->line[start]); start--)
			;

		if(start < 0)
			start = 0;
		else /* !isfnamechar(line[start]) */
			start++;

	}else{
		/* look forwards for the beginning */
		for(; !isfnamechar(l->line[start]) && (unsigned)start < l->len_line; start++)
			;

		if((unsigned)start == l->len_line)
			return NULL;
	}


	int end = start;
	for(; (unsigned)end < l->len_line && isfnamechar(l->line[end]); end++)
		;

	/* end is either at the end of the line or not a fnamechar,
	 * either way: */
	end--;

	/* avoid '.' at the end */
	if(end > 0 && l->line[end] == '.')
		end--;

	if(end < start)
		return NULL;

	return ustrdup_len(&l->line[start], end - start + 1);
}

void list_clear_flag(list_t *l)
{
	for(; l; l = l->next)
		l->flag = 0;
}

void list_flag_range(list_t *l, const struct range *r, int v)
{
	size_t n = r->end - r->start + 1;
	for(l = list_seek(l, r->start, false);
	    l && n > 0;
	    l = l->next, n--)
	{
		l->flag = v;
	}
}

list_t *list_flagfind(list_t *l, int v, int *py)
{
	*py = 0;
	for(; l; l = l->next, ++*py)
		if(l->flag == v)
			return l;
	return NULL;
}

list_t *list_contains(list_t *l, const char *str)
{
	for(; l; l = l->next)
		if(!strcmp(l->line, str))
			return l;
	return NULL;
}
