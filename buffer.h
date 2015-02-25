#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <time.h> /* time_t */

#include "bufmode.h"

#include "yank.h"

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

	/* used for `gv' */
	struct
	{
		point_t npos, vpos;
		enum buf_mode mode;
	} prev_visual;

	point_t ui_start;     /* offset into buffer */
	rect_t  screen_coord; /* buffer pos in screen */

	buffer_t *neighbours[4];

	char *fname;
	bool eol;
	bool modified;
	time_t mtime;

	enum buf_mode ui_mode;

	unsigned col_insert_height;
};

buffer_t *buffer_new(void);
void buffer_new_fname(buffer_t **, const char *, int *err);

int buffer_setmode(buffer_t *, enum buf_mode m); /* 0 = success */
void buffer_togglev(buffer_t *, bool corner_toggle);

void buffer_free(buffer_t *);

int buffer_replace_fname(buffer_t *, const char *);
int buffer_write_file(buffer_t *, int n, FILE *, bool eol);

void buffer_set_fname(buffer_t *, const char *);
const char *buffer_fname(const buffer_t *);

void buffer_inschar(buffer_t *, char ch);
void buffer_inschar_at(buffer_t *, char ch, int *x, int *y);
/* TODO: remove arg 2 and 3 */
void buffer_delchar(buffer_t *, int *x, int *y);

void buffer_insstr(buffer_t *, const char *, size_t);

typedef void buffer_action_f(
		buffer_t *, const region_t *, point_t *out);

struct buffer_action
{
	buffer_action_f *fn;
	bool always_linewise;
};

extern struct buffer_action
	buffer_delregion, buffer_joinregion, buffer_yankregion,
	buffer_indent, buffer_unindent;

void buffer_insyank(buffer_t *, const yank *, bool prepend, bool modify);

int buffer_filter(
		buffer_t *,
		const region_t *,
		const char *cmd);

void buffer_insline(buffer_t *, int dir);

list_t *buffer_current_line(const buffer_t *);

unsigned buffer_nlines(const buffer_t *);

void buffer_caseregion(
		buffer_t *, enum case_tog,
		const region_t *region);

/* positioning */
buffer_t *buffer_topleftmost(buffer_t *b);
void buffer_add_neighbour(buffer_t *to, enum buffer_neighbour, buffer_t *new);

const char *buffer_shortfname(const char *); /* internal fname buffer */

bool buffer_findat(const buffer_t *, const char *, point_t *, int dir);

point_t buffer_toscreen(const buffer_t *, point_t const *);
point_t *buffer_uipos_alt(buffer_t *buf);

#endif
