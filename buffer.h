#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer
{
	list_t *head;
} buffer_t;

buffer_t *buffer_new(void);
buffer_t *buffer_new_fname(const char *);
buffer_t *buffer_new_file(FILE *);

void buffer_inschar(buffer_t *, int x, int y, char ch);
void buffer_delchar(buffer_t *, int x, int y);

#endif
