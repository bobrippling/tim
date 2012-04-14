#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer buffer_t;

enum buffer_neighbour
{
	BUF_LEFT,
	BUF_RIGHT,
	BUF_UP,
	BUF_DOWN
};

struct buffer
{
	list_t *head;

	point_t ui_pos;       /* cursor pos in buffer */
	point_t ui_start;     /* offset into buffer */
	rect_t  screen_coord; /* buffer pos in screen */

	buffer_t *neighbours[4];
};

buffer_t *buffer_new(void);
buffer_t *buffer_new_fname(const char *);
buffer_t *buffer_new_file(FILE *);

void buffer_free(buffer_t *);

void buffer_inschar(buffer_t *, int *x, int *y, char ch);
void buffer_delchar(buffer_t *, int *x, int *y);

/* positioning */
buffer_t *buffer_topleftmost(buffer_t *b);
void buffer_add_neighbour(buffer_t *to, enum buffer_neighbour, buffer_t *new);

#endif
