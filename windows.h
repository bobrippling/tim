#ifndef WINDOWS_H
#define WINDOWS_H

window *windows_cur(void);
void windows_set_cur(window *);

#define ITER_WINDOWS_FROM(win, from)                  \
	win = window_topleftmost(from);                     \
	for(window *h = win; h; h = h->neighbours.right)    \
		for(win = h; win; win = win->neighbours.below)

#define ITER_WINDOWS(win) ITER_WINDOWS_FROM(win, windows_cur())

#endif
