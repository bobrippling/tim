#ifndef WINDOWS_H
#define WINDOWS_H

window *windows_cur(void);
void windows_set_cur(window *);

#define ITER_WINDOWS(win)                             \
	for(win = windows_cur();                            \
			win->neighbours.above;                          \
			win = win->neighbours.above);                   \
	for(; win->neighbours.left;                         \
			win = win->neighbours.left);                    \
	for(window *h = win; h; h = h->neighbours.right)    \
		for(win = h; win; win = win->neighbours.below)

/* argument list */
char *windows_next_fname(bool pop);

enum windows_init_args
{
	WIN_NONE = 0,
	WIN_VALL,
	WIN_HALL
};

void windows_init(int argc, char **argv, enum windows_init_args a, unsigned off);
void windows_term(void);

#endif
