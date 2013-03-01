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

	char *fname;
};

buffer_t *buffer_new(void);
void buffer_new_fname(buffer_t **, const char *, int *err);

void buffer_free(buffer_t *);

int buffer_replace_file( buffer_t *, FILE *);
int buffer_replace_fname(buffer_t *, const char *);

void buffer_set_fname(buffer_t *, const char *);
const char *buffer_fname(const buffer_t *);

/* TODO: remove arg 2 and 3 */
void buffer_inschar(buffer_t *, int *x, int *y, char ch);
void buffer_delchar(buffer_t *, int *x, int *y);

void buffer_insline(buffer_t *, int dir);

struct list *buffer_current_line(const buffer_t *);

/* positioning */
buffer_t *buffer_topleftmost(buffer_t *b);
void buffer_add_neighbour(buffer_t *to, enum buffer_neighbour, buffer_t *new);

#endif
