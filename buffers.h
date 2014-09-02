#ifndef BUFFERS_H
#define BUFFERS_H

enum buffer_init_args
{
	BUF_NONE = 0,
	BUF_VALL,
	BUF_HALL
};

void buffers_init(int argc, char **argv, enum buffer_init_args a, unsigned off);
void buffers_term(void);

buffer_t *buffers_cur(void);
void buffers_set_cur(buffer_t *);

/* argument list */
char *buffers_next_fname(void);

#define ITER_BUFFERS(b)                               \
	for(buf = buffers_cur();                            \
			buf->neighbours.above;                          \
			buf = buf->neighbours.above);                  \
	for(; buf->neighbours.left;                         \
			buf = buf->neighbours.left);                   \
	for(buffer_t *h = buf; h; h = h->neighbours.right)  \
		for(buf = h; buf; buf = buf->neighbours.below)

#endif
