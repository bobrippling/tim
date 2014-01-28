#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "word.h"

enum word_state word_state(
		const list_t *l, int x, bool big_words)
{
	if(l && 0 <= x && (unsigned)x < l->len_line){
		const char ch = l->line[x];

		if(isspace(ch))
			return W_SPACE;
		else if(big_words)
			return W_KEYWORD;
		else if(isalnum(ch) || ch == '_')
			return W_KEYWORD;
		return W_NON_KWORD;
	}

	return W_NONE;
}

list_t *word_seek(
		list_t *l, int dir, point_t *to,
		enum word_state target, bool big_words)
{
	while((word_state(l, to->x, big_words) & target) == 0){
		const int y = to->y;

		l = list_advance_x(l, dir, &to->y, &to->x);

		if(!l)
			break;
		if(y != to->y)
			break; /* newlines are a fresh start */
	}
	return l;
}

void word_seek_to_end(list_t *l, int dir, point_t *to, bool big_words)
{
	/* put us on the start/end of the word */
	const enum word_state st = word_state(l, to->x, big_words);

	if(st != W_NONE)
		while(word_state(l, to->x + dir, big_words) == st)
			to->x += dir;
}
