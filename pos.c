#include "pos.h"

#define SWAP(a, b) do{ int t = a; a = b; b = t; }while(0)

void point_sort(point_t *a, point_t *b)
{
	/* need A to be before B */
	if(a->y > b->y)
		SWAP(a->y, b->y);

	if(a->x > b->x)
		SWAP(a->x, b->x);
}
