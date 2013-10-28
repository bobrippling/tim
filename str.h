#ifndef STR_H
#define STR_H

#include <stdbool.h>

char *strchr_rev(const char *s, int ch, const char *start);
bool isallspace(const char *);
bool paren_match(char c, char *other);
bool paren_left(char);

void str_ltrim(char *, size_t *);
void str_rtrim(char *, size_t *);

#endif
