#ifndef WINDOW_H
#define WINDOW_H

typedef struct window window;

struct window
{
	buffer_t *buf;

	point_t ui_start;     /* offset into buffer */
	rect_t  screen_coord; /* window pos in screen */

	point_t ui_npos;  /* cursor pos in buffer */
	point_t ui_vpos;  /* when in visual mode - other point */
	point_t *ui_pos; /* which one is in use? */
	point_t ui_paren; /* paren matching - { -1, -1 } if none */

	enum buf_mode ui_mode;

	struct
	{
		window *left, *right, *below, *above;
	} neighbours;
};

enum neighbour
{
	neighbour_up, neighbour_down, neighbour_left, neighbour_right
};

bool neighbour_is_vertical(enum neighbour);

window *window_topleftmost(window *b);
window *window_next(window *);

void window_add_neighbour(window *, enum neighbour dir, window *);
void window_evict(window *evictee);

void window_replace_buffer(window *, buffer_t *);

bool window_replace_fname(
		window *win, const char *fname, const char **err);

bool window_reload_buffer(window *win, const char **const err);

void window_inschar(window *win, char ch);

list_t *window_current_line(const window *w, bool create);
char *window_current_word(const window *);
char *window_current_fname(const window *);

point_t window_toscreen(const window *, point_t const *);

void window_togglev(window *, bool corner_toggle);

point_t *window_uipos_alt(window *);

int window_setmode(window *, enum buf_mode m); /* 0 = success */

void window_calc_rects(
		window *topleft,
		const unsigned screenwidth,
		const unsigned screenheight);

void window_smartindent(window *win);

window *window_new(buffer_t *);
void window_free(window *);

#endif
