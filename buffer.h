#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>

typedef struct buffer buffer_t;

enum buffer_neighbour
{
	BUF_LEFT,
	BUF_RIGHT,
	BUF_UP,
	BUF_DOWN
};

enum case_tog
{
	CASE_TOGGLE,
	CASE_UPPER,
	CASE_LOWER,
};

struct buffer
{
	list_t *head;

	point_t ui_npos;  /* cursor pos in buffer */
	point_t ui_vpos;  /* when in visual mode - other point */
	point_t *ui_pos; /* which one is in use? */

	point_t ui_start;     /* offset into buffer */
	rect_t  screen_coord; /* buffer pos in screen */

	buffer_t *neighbours[4];

	char *fname;

	enum buf_mode
	{
		UI_NORMAL = 1 << 0,

		UI_INSERT = 1 << 1,
		UI_INSERT_COL = 1 << 2,

		UI_VISUAL_CHAR = 1 << 3,
		UI_VISUAL_COL = 1 << 4,
		UI_VISUAL_LN = 1 << 5,

#define UI_VISUAL_ANY (\
		UI_VISUAL_CHAR | \
		UI_VISUAL_COL  | \
		UI_VISUAL_LN)

#define UI_INSERT_ANY (UI_INSERT | UI_INSERT_COL)
	} ui_mode;

	unsigned col_insert_height;
};

buffer_t *buffer_new(void);
void buffer_new_fname(buffer_t **, const char *, int *err);

int buffer_setmode(buffer_t *, enum buf_mode m); /* 0 = success */
void buffer_togglev(buffer_t *);

void buffer_free(buffer_t *);

int buffer_replace_file( buffer_t *, FILE *);
int buffer_replace_fname(buffer_t *, const char *);

void buffer_set_fname(buffer_t *, const char *);
const char *buffer_fname(const buffer_t *);

/* TODO: remove arg 2 and 3 */
void buffer_inschar(buffer_t *, int *x, int *y, char ch);
void buffer_delchar(buffer_t *, int *x, int *y);

typedef void buffer_action_f(
		buffer_t *, const region_t *, point_t *out);

struct buffer_action
{
	buffer_action_f *fn;
	int is_linewise;
};

struct buffer_action
	buffer_delregion, buffer_joinregion,
	buffer_indent, buffer_unindent;

void buffer_replace_chars(buffer_t *, int ch, unsigned n);

void buffer_insline(buffer_t *, int dir);

list_t *buffer_current_line(const buffer_t *);

unsigned buffer_nlines(const buffer_t *);

void buffer_case(
		buffer_t *, enum case_tog,
		unsigned repeat);

/* positioning */
buffer_t *buffer_topleftmost(buffer_t *b);
void buffer_add_neighbour(buffer_t *to, enum buffer_neighbour, buffer_t *new);

const char *buffer_shortfname(const char *); /* internal fname buffer */

int buffer_find(const buffer_t *, const char *, point_t *, int rev);

point_t buffer_toscreen(const buffer_t *, point_t const *);
point_t *buffer_uipos_alt(buffer_t *buf);

#endif
