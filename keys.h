#ifndef KEYS_H
#define KEYS_H

typedef struct Key
{
	char ch;
	void (*func)(void);
} Key;

void k_cmd(void);

#endif
