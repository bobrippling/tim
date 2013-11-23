#ifndef MEM_H
#define MEM_H

void *umalloc(size_t);
void *urealloc(void *, size_t);
char *ustrdup(const char *);
char *ustrdup_len(const char *s, size_t len);
char *join(const char *sep, const char **, int);

#endif
