#ifndef STR_H
#define STR_H

char *strchr_rev(const char *s, int ch, const char *start);
int   isallspace(const char *);

void str_ltrim(char *, size_t *);
void str_rtrim(char *, size_t *);

#endif
