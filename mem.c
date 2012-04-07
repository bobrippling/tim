#include <stdlib.h>

#include "mem.h"

void *umalloc(size_t l)
{
	void *p = malloc(l);
	if(!p)
		abort();
	return p;
}

void *urealloc(void *p, size_t l)
{
	void *r = realloc(p, l);
	if(!r)
		abort();
	return r;
}
