#ifndef KEYS_H
#define KEYS_H

#define CTRL_AND(c)  ((c) & 037)

typedef struct Key
{
	char ch;
	void (*func)(void);
} Key;

typedef void key_func(void);

key_func k_cmd, k_ins;

key_func k_down, k_up, k_right, k_left;

key_func k_redraw;

#endif
