#ifndef KEYS_H
#define KEYS_H

typedef union keyarg_u keyarg_u;

typedef void key_func(const keyarg_u *, unsigned repeat, const int ch);

union keyarg_u
{
	int i;
	struct
	{
		motion m;
		unsigned repeat;
	} motion;
	struct filter_input
	{
		enum
		{
			FILTER_SELF, FILTER_CMD
		} type;
		char *s;
	} filter;
	point_t pos;
};

typedef struct nkey_t
{
	const char *str;
	key_func *func;
	keyarg_u arg;
	enum buf_mode mode;
} nkey_t;

typedef struct motionkey_t
{
	const char *keys;
	motion motion;
} motionkey_t;

/* returns 0 on success */
const motion *motion_read(unsigned *repeat, bool apply_maps);

int keys_filter(
		enum io io_m,
		char *struc, unsigned n_ents,
		unsigned st_off, unsigned st_sz,
		int mode_off);

key_func k_cmd, k_set_mode;
key_func k_escape;
key_func k_redraw;
key_func k_scroll;
key_func k_winsel;
key_func k_show;
key_func k_open;
key_func k_del, k_change;
key_func k_motion;
key_func k_replace;
key_func k_join;
key_func k_indent;
key_func k_case;
key_func k_filter;
key_func k_ins_colcopy;

key_func k_vtoggle;

#endif
