#include <stdlib.h>
#include <string.h>

#include "mem.h"

void *umalloc(size_t l)
{
	void *p = malloc(l);
	if(!p)
		abort();
	memset(p, 0, l);
	return p;
}

void *urealloc(void *p, size_t l)
{
	void *r = realloc(p, l);
	if(!r)
		abort();
	return r;
}

char *ustrdup(const char *s)
{
	char *r = umalloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}
