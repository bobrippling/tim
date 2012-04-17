#ifndef KEYS_H
#define KEYS_H

#define key_esc '\x1b'

typedef union KeyArg KeyArg;

typedef void key_func(const KeyArg *);

union KeyArg
{
	int i;
	char *s;
	struct
	{
		int x, y;
	} pos;
};

typedef struct Key
{
	char ch;
	key_func *func;
	KeyArg arg;
	enum ui_mode mode;
} Key;

typedef struct MotionKey
{
	char ch;
	motion_func *func;
	Motion motion;
	enum ui_mode mode;
} MotionKey;

key_func k_cmd, k_set_mode;
key_func k_redraw;
key_func k_scroll;
key_func k_winsel;
key_func k_show;
key_func k_open;

#endif
