#include "pos.h"

#define SWAP(T, a, b) do{ T t = a; a = b; b = t; }while(0)

void point_sort_y(point_t *a, point_t *b)
{
	if(a->y > b->y)
		SWAP(point_t, *a, *b);
}

void point_sort_all(point_t *a, point_t *b)
{
	point_sort_y(a, b);

	if(a->x > b->x)
		SWAP(int, a->x, b->x);
}
