#ifndef STR_H
#define STR_H

#include <stdbool.h>

char *strchr_rev(const char *s, int ch, const char *start);
bool isallspace(const char *);
bool paren_match(char c, char *other);
bool paren_left(char);
char paren_opposite(char);

char *skipspace(const char *);

void str_ltrim(char *, size_t *);
void str_rtrim(char *, size_t *);

char *tim_strrevstr(
		char *restrict haystack,
		unsigned off,
		const char *restrict needle);

char *tim_strstr(
		char *restrict haystack,
		size_t len,
		const char *restrict needle);

char *expand_tilde(const char *);

#endif
