#ifndef MAP_H
#define MAP_H

typedef struct keymap
{
	enum io mode;
	char from;
	const char *to;
} keymap_t;

#endif
