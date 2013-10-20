#ifndef KEYS_H
#define KEYS_H

typedef union keyarg_u keyarg_u;

typedef void key_func(const keyarg_u *, unsigned repeat, const int ch);

union keyarg_u
{
	int i;
	motion motion;
	char *s;
	point_t pos;
};

typedef struct nkey_t
{
	char ch;
	key_func *func;
	keyarg_u arg;
	enum ui_mode mode;
} nkey_t;

typedef struct motionkey_t
{
	char ch;
	motion motion;
} motionkey_t;

const motion *motion_find(int first_ch, int skip);
int motion_repeat_read(motion_repeat *, int *pfirst_ch, int skip);

key_func k_cmd, k_set_mode;
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

key_func k_vtoggle;

#endif
