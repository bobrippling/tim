#ifndef KEYS_H
#define KEYS_H

#define key_esc '\x1b'

typedef union keyarg_u keyarg_u;

typedef void key_func(const keyarg_u *, unsigned repeat);

union keyarg_u
{
	int i;
	enum { LINEWISE = 1 << 0, EXCLUSIVE = 1 << 1 } how;
	motion motion;
	char *s;
	point_t pos;
};

typedef struct ikey_t
{
	char ch;
	key_func *func;
	keyarg_u arg;
	enum ui_mode mode;
} ikey_t;

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
key_func k_del;
key_func k_motion;

#endif
