#ifndef KEYS_H
#define KEYS_H

#define CTRL_AND(c)  ((c) & 037)

typedef union KeyArg
{
	int i;
	struct
	{
		int x, y;
	} pos;
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

key_func k_move; /* l, r, u, d */
key_func k_sol, k_eol, k_sof, k_eof;

key_func k_redraw;

#define key_esc '\x1b'

#endif
