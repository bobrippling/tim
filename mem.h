#ifndef MEM_H
#define MEM_H

void *umalloc(size_t);
void *urealloc(void *, size_t);
char *ustrdup(const char *);
char *ustrdup_len(const char *, size_t);
char *ustrvprintf(const char *fmt, va_list l);
char *join(const char *sep, char **, int);

#endif
