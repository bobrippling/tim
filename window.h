#ifndef WINDOW_H
#define WINDOW_H

typedef struct
{
	buffer_t *buf;
	unsigned retains;
} buffer_retained;

typedef struct window window;

struct window
{
	buffer_retained buf;

	struct
	{
		window *left, *right, *below, *above;
	} neighbours;
};

window *window_topleftmost(window *b);
void window_add_neighbour(window *, bool splitright, window *);
void window_evict(window *evictee);

#endif
