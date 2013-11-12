#ifndef KEYS_H
#define KEYS_H

#include <stdbool.h>
#include "surround.h"

typedef union keyarg_u keyarg_u;

typedef void key_func(const keyarg_u *, unsigned repeat, const int ch);

union keyarg_u
{
	int i;
	bool b;
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
	struct
	{
		cmd_t fn;
		const char **argv;
		bool force;
	} cmd;
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

const motion *motion_read(unsigned *repeat, bool apply_maps);
const surround_key_t *surround_read(int surround_ch);

int keys_filter(
		enum io io_m,
		char *struc, unsigned n_ents,
		unsigned st_off, unsigned st_sz,
		int mode_off);

key_func k_prompt_cmd, k_docmd, k_set_mode;
key_func k_escape;
key_func k_redraw;
key_func k_scroll;
key_func k_winsel;
key_func k_show;
key_func k_open;
key_func k_del, k_change, k_yank;
key_func k_motion;
key_func k_replace;
key_func k_join;
key_func k_indent;
key_func k_put;
key_func k_case;
key_func k_filter;
key_func k_ins_colcopy;
key_func k_normal1;

key_func k_vtoggle;
key_func k_go_visual;
key_func k_go_insert;

#endif
