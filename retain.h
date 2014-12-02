#ifndef RETAIN_H
#define RETAIN_H

struct retain
{
	int rcount;
};

void *retain_nochk(struct retain *);
void release_nochk(struct retain *, void (*fn)(void *));

#define retain(x) ((__typeof(x))retain_nochk((struct retain *)x))
#define release(x, fn) release_nochk((struct retain *)x, (void (*)(void *))fn)

#define retain_init(x) (((struct retain *)(x))->rcount = 1)

#endif
