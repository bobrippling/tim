#include "retain.h"

void *retain_nochk(struct retain *r)
{
	if(r)
		r->rcount++;
	return r;
}

void release_nochk(struct retain *r, void (*fn)(void *))
{
	if(--r->rcount == 0)
		fn(r);
}
