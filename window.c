#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "mem.h"
#include "macros.h"

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "window.h"

#include "ncurses.h"

void window_calc_rects(
		window *topleft,
		const unsigned screenwidth,
		const unsigned screenheight)
{
	unsigned nwins_h = 0;
	window *win_h;
	for(win_h = topleft; win_h; win_h = win_h->neighbours.right, nwins_h++);

	const unsigned nvlines = nwins_h + 1;
	const unsigned winspace_v = screenwidth - nvlines;
	unsigned winwidth = winspace_v / nwins_h;

	unsigned i_h;
	for(i_h = 0, win_h = topleft; win_h; win_h = win_h->neighbours.right, i_h++){
		unsigned nwins_v = 0;
		window *win_v;
		for(win_v = win_h; win_v; win_v = win_v->neighbours.below)
			nwins_v++;

		/* final win gets any left over cols */
		if(!win_h->neighbours.right)
			winwidth += winspace_v % nwins_h;

		const unsigned nhlines = nwins_v - 1;
		const unsigned winspace_h = screenheight - /*status*/1 - nhlines;
		const unsigned winheight = winspace_h / nwins_v;

		unsigned i_v;
		for(i_v = 0, win_v = win_h; win_v; win_v = win_v->neighbours.below, i_v++){
			rect_t r = {
				.x = i_h * (winwidth + 1),
				.y = i_v * (winheight + 1),
				.w = winwidth,
				.h = winheight,
			};

			/* final win gets any left over lines */
			if(!win_v->neighbours.below)
				r.h += winspace_h % nwins_v;

			win_v->screen_coord = r;
		}
	}
}

window *window_next(window *cur)
{
#define RET_IF(x) if(x) return x
	RET_IF(cur->neighbours.below);
	RET_IF(cur->neighbours.above);
	RET_IF(cur->neighbours.right);
	RET_IF(cur->neighbours.left);
#undef RET_IF
	return NULL;
}

window *window_topleftmost(window *win)
{
	for(;;){
		int changed = 0;
		for(; win->neighbours.left;  changed = 1, win = win->neighbours.left);
		for(; win->neighbours.above; changed = 1, win = win->neighbours.above);
		if(!changed)
			break;
	}

	return win;
}

void window_evict(window *const evictee)
{
	if(evictee->neighbours.above){
		/* we are just a child in the chain */
		assert(!evictee->neighbours.left);
		assert(!evictee->neighbours.right);

		window *above = evictee->neighbours.above;

		assert(above->neighbours.below == evictee);
		above->neighbours.below = evictee->neighbours.below;

		if(evictee->neighbours.below)
			evictee->neighbours.below->neighbours.above = above;

		evictee->neighbours.above = evictee->neighbours.below = NULL;
		return;
	}

	/* we are a top-most chain.
	 * remove ourselves from the left-right hierarchy
	 * if we have a child below, it takes our place.
	 */
	window *const child = evictee->neighbours.below;

	if(evictee->neighbours.left){
		window *left = evictee->neighbours.left;

		assert(left->neighbours.right == evictee);

		window *target = child ? child : evictee->neighbours.right;
		left->neighbours.right = target;
		if(target)
			target->neighbours.left = left;
	}

	if(evictee->neighbours.right){
		window *right = evictee->neighbours.right;

		window *target = child ? child : evictee->neighbours.left;
		right->neighbours.left = target;
		if(target)
			target->neighbours.right = right;
	}

	evictee->neighbours.left = evictee->neighbours.right = NULL;
	evictee->neighbours.above = evictee->neighbours.below = NULL;
	if(child){
		assert(child->neighbours.above == evictee);
		child->neighbours.above = NULL;
	}
}

void window_add_neighbour(window *to, enum neighbour dir, window *new)
{
	window_evict(new);

	switch(dir){
		case neighbour_left:
		case neighbour_right:
		{
			window *topmost;
			for(topmost = to; topmost->neighbours.above; topmost = topmost->neighbours.above);

			window **pdest = (dir == neighbour_right
					? &topmost->neighbours.right
					: &topmost->neighbours.left);

			window *dest = *pdest;

			if(!dest){
				*pdest = new;
				if(dir == neighbour_right)
					new->neighbours.left = topmost;
				else
					new->neighbours.right = topmost;
			}else{
				/* walk down until we find an entry where we left */
				while(dest->neighbours.below
				&& dest->ui_start.y <= new->ui_start.y)
				{
					dest = dest->neighbours.below;
				}

				window *replace = dest->neighbours.below;
				dest->neighbours.below = new;
				if(replace)
					replace->neighbours.above = new;

				new->neighbours.above = dest;
				new->neighbours.below = replace;
			}
			break;
		}
		case neighbour_down:
		{
			window *evicted = to->neighbours.below;

			to->neighbours.below = new;
			new->neighbours.above = to;

			new->neighbours.below = evicted;
			if(evicted)
				evicted->neighbours.above = new;
			break;
		}
		case neighbour_up:
		{
			window *up = to->neighbours.above;
			if(up){
				/* insert in the chain */
				new->neighbours.above = up;
				up->neighbours.below = new;

				to->neighbours.above = new;
				new->neighbours.below = to;
			}else{
				/* rewrite left and right pointers */
				new->neighbours.left = to->neighbours.left;
				to->neighbours.left = NULL;
				new->neighbours.right = to->neighbours.right;
				to->neighbours.right = NULL;

				if(new->neighbours.right)
					new->neighbours.right->neighbours.left = new;
				if(new->neighbours.left)
					new->neighbours.left->neighbours.right = new;

				new->neighbours.below = to;
				to->neighbours.above = new;
			}
			break;
		}
	}
}

window *window_new(buffer_t *b)
{
	window *win = umalloc(sizeof *win);

	win->buf = retain(b);

	win->ui_pos = &win->ui_npos;
	win->ui_mode = UI_NORMAL;

	return win;
}

void window_free(window *w)
{
	buffer_release(w->buf);
	free(w);
}

void window_togglev(window *win, bool corner_toggle)
{
	if(corner_toggle){
		point_t *alt = window_uipos_alt(win);

		int tmp = win->ui_pos->x;
		win->ui_pos->x = alt->x;
		alt->x = tmp;
	}else{
		win->ui_pos = (win->ui_pos == &win->ui_npos)
			? &win->ui_vpos
			: &win->ui_npos;
	}
}

list_t *window_current_line(const window *w, bool create)
{
	return list_seek(w->buf->head, w->ui_pos->y, create);
}

point_t window_toscreen(const window *win, point_t const *pt)
{
	list_t *l = list_seek(win->buf->head, win->ui_pos->y, 0);
	int xoff = 0;

	if(l && l->len_line > 0)
		for(int x = MIN((unsigned)win->ui_pos->x, l->len_line - 1);
				x >= 0;
				x--)
			xoff += nc_charlen(l->line[x]) - 1;

	return (point_t){
		win->screen_coord.x + pt->x - win->ui_start.x + xoff,
		win->screen_coord.y + pt->y - win->ui_start.y
	};
}

int window_setmode(window *win, enum buf_mode m)
{
	if(!m || (m & (m - 1))){
		return -1;
	}else{
		if(win->ui_mode & UI_INSERT_ANY)
			win->buf->prev_insert = *win->ui_pos;

		if(m & UI_VISUAL_ANY){
			if((win->ui_mode & UI_VISUAL_ANY) == 0){
				/* from non-visual to visual */
				*window_uipos_alt(win) = *win->ui_pos;
			}
		}else{
			/* from a visual, save state */
			win->buf->prev_visual.mode = win->ui_mode;

			win->buf->prev_visual.npos = *win->ui_pos;
			win->buf->prev_visual.vpos = *window_uipos_alt(win);
		}

		win->ui_mode = m;
		return 0;
	}
}

void window_inschar(window *win, char ch)
{
	if(win->ui_mode != UI_INSERT_COL || isnewline(ch)){
		/* can't col-insert a newline, revert */
		if(win->ui_mode == UI_INSERT_COL)
			window_setmode(win, UI_INSERT);

		buffer_inscolchar(win->buf, ch, 1, win->ui_pos);
	}else{
		buffer_inscolchar(win->buf, ch, win->buf->col_insert_height, win->ui_pos);
	}
}

point_t *window_uipos_alt(window *win)
{
	if(win->ui_pos == &win->ui_vpos)
		return &win->ui_npos;
	return &win->ui_vpos;
}

void window_replace_buffer(window *win, buffer_t *buf)
{
	buffer_release(win->buf);
	win->buf = retain(buf);
	win->ui_pos->y = MIN(win->ui_pos->y, list_count(buf->head));
}

bool window_replace_fname(window *win, const char *fname, const char **const err)
{
	buffer_t *buf = NULL;
	buffer_new_fname(&buf, fname, err);

	if(!*err){
		window_replace_buffer(win, buf);
		buffer_release(buf);
	}

	return !*err;
}

bool window_reload_buffer(window *win, const char **const err)
{
	const char *fname = buffer_fname(win->buf);
	if(!fname)
		return false;

	/* need to update the buffer's list, not the window,
	 * since there may be multiple windows with the old buffer */
	buffer_replace_fname(win->buf, fname, err);

	return !*err;
}
