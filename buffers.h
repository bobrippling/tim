#ifndef BUFFERS_H
#define BUFFERS_H

enum buffer_init_args
{
	BUF_NONE = 0,
	BUF_VALL,
	BUF_HALL
};

void buffers_init(int argc, char **argv, enum buffer_init_args a);

buffer_t *buffers_cur(void);
void buffers_set_cur(buffer_t *);

#endif
