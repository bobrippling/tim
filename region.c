#include <stdbool.h>

#include "pos.h"
#include "region.h"

bool region_contains(const region_t *re, point_t const *pt)
{
	region_t sorted = *re;
	point_sort_full(&sorted.begin, &sorted.end);

	const struct
	{
		bool x, y;
	} in = {
		sorted.begin.x <= pt->x && pt->x <= sorted.end.x,
		sorted.begin.y <= pt->y && pt->y <= sorted.end.y
	};

	switch(re->type){
		case REGION_LINE:
			return in.y;

		case REGION_COL:
			return in.y && in.x;

		case REGION_CHAR:
			if(!in.y)
				return false;

			if(re->begin.y == re->end.y)
				return in.x;

			sorted = *re;
			point_sort_y(&sorted.begin, &sorted.end);

			if(pt->y == sorted.begin.y)
				return pt->x >= sorted.begin.x;
			if(pt->y == sorted.end.y)
				return pt->x <= sorted.end.x;

			return true;
	}

	return false;
}
