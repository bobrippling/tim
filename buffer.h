#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <time.h> /* time_t */

#include "bufmode.h"

#include "yank.h"
#include "retain.h"

typedef struct buffer buffer_t;

enum case_tog
{
	CASE_TOGGLE,
	CASE_UPPER,
	CASE_LOWER,
};

struct buffer
{
	/* must be first */
	struct retain retains;

	list_t *head;

	/* used for `gv' */
	struct
	{
		point_t npos, vpos;
		enum buf_mode mode;
	} prev_visual;
	point_t prev_insert;

	char *fname;
	bool eol;
	bool modified;
	time_t mtime;

	unsigned col_insert_height;
};

buffer_t *buffer_new(void);

void buffer_new_fname(
		buffer_t **, const char *fname,
		const char **const err);

void buffer_replace_fname(
		buffer_t *, const char *fname,
		const char **const err);

buffer_t *buffer_new_file_nofind(FILE *f);

int buffer_opencount(const buffer_t *);

void buffer_free(buffer_t *);

int buffer_write_file(buffer_t *, int n, FILE *, bool eol);

void buffer_set_fname(buffer_t *, const char *);
const char *buffer_fname(const buffer_t *);

void buffer_inscolchar(
		buffer_t *buf, char ch, unsigned ncols,
		point_t *const ui_pos);

void buffer_inschar_at(buffer_t *, char ch, int *x, int *y);

/* TODO: remove arg 2 and 3 */
void buffer_delchar(buffer_t *, int *x, int *y);

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

void buffer_smartindent(buffer_t *, int *x, int y);

void buffer_insyank(
		buffer_t *, const yank *,
		point_t *ui_pos,
		bool prepend, bool modify);

int buffer_filter(
		buffer_t *,
		const region_t *,
		const char *cmd);

void buffer_insline(buffer_t *, int dir, point_t *ui_pos);

unsigned buffer_nlines(const buffer_t *);

void buffer_caseregion(
		buffer_t *, enum case_tog,
		const region_t *region);

const char *buffer_shortfname(const char *); /* internal fname buffer */

bool buffer_findat(const buffer_t *, const char *, point_t *, int dir);

#define buffer_release(b) release((b), buffer_free)

#endif
