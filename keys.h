#ifndef KEYS_H
#define KEYS_H

#define key_esc '\x1b'

typedef union keyarg_u keyarg_u;

typedef void key_func(const keyarg_u *);

union keyarg_u
{
	int i;
	char *s;
	point_t pos;
};

typedef struct key_t
{
	char ch;
	key_func *func;
	keyarg_u arg;
	enum ui_mode mode;
} key_t;

typedef struct motionkey_t
{
	char ch;
	motion motion;
	enum ui_mode mode;
} motionkey_t;

motionkey_t *motion_next(enum ui_mode mode, int ch, int skip);

key_func k_cmd, k_set_mode;
key_func k_redraw;
key_func k_scroll;
key_func k_winsel;
key_func k_show;
key_func k_open;
key_func k_del;

#endif
