#ifndef SURROUND_H
#define SURROUND_H

typedef bool surround_func(
		char arg, unsigned repeat,
		window *, region_t *);

typedef struct surround_key
{
	enum region_type type;
	const char *matches;
	surround_func *func;
} surround_key_t;

surround_func surround_paren;
surround_func surround_quote;
surround_func surround_block;
surround_func surround_para;
surround_func surround_word;

bool surround_apply(const surround_key_t *k,
		char in_around, char type, unsigned repeat,
		window *win, region_t *r);

#define surround_beginning_char(ch) ((ch) == 'a' || (ch) == 'i')
#define SURROUND_IN_CHAR 'i'

#endif
