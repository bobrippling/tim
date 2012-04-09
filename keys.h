#ifndef KEYS_H
#define KEYS_H

#define CTRL_AND(c)  ((c) & 037)

typedef union KeyArg
{
	int i;
} KeyArg;

typedef struct Key
{
	char ch;
	void (*func)(KeyArg *);
	KeyArg arg;
	enum ui_mode mode;
} Key;

typedef void key_func(KeyArg *);

key_func k_cmd, k_set_mode;

key_func k_down, k_up, k_right, k_left;

key_func k_redraw;

#define key_esc '\x1b'

#endif
