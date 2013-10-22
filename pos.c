#include "pos.h"

#define SWAP(T, a, b) do{ T t = a; a = b; b = t; }while(0)

void point_sort_y(point_t *const a, point_t *const b)
{
	if(a->y > b->y)
		SWAP(point_t, *a, *b);
}

void point_sort_full(point_t *const a, point_t *const b)
{
	point_sort_y(a, b);

	if(a->x > b->x)
		SWAP(int, a->x, b->x);
}

void point_sort_yx(point_t *const a, point_t *const b)
{
	if(b->y < a->y){
		point_t tmp = *b;
		*b = *a;
		*a = tmp;
	}else if(b->y == a->y && b->x < a->x){
		int tmp = b->x;
		b->x = a->x;
		a->x = tmp;
	}
}
