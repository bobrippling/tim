#ifndef WORD_H
#define WORD_H

enum word_state
{
	W_NONE = 0,
	W_SPACE = 1 << 0,
	W_KEYWORD = 1 << 1,
	W_NON_KWORD = 1 << 2,
};

enum word_state word_state(
		const list_t *l, int x, bool big_words);

list_t *word_seek(
		list_t *l, int dir, point_t *to,
		enum word_state target, bool big_words);

void word_seek_to_end(
		list_t *l, int dir, point_t *to, bool big_words);

#endif
