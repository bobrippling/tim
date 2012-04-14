#ifndef BUFFERS_H
#define BUFFERS_H

void buffers_init(int argc, char **argv);

buffer_t *buffers_cur(void);
void buffers_set_cur(buffer_t *);

#endif
