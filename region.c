#include <stdbool.h>

#include "pos.h"
#include "region.h"

bool region_contains(const region_t *re, int x, int y)
{
	region_t sorted = *re;
	point_sort_full(&sorted.begin, &sorted.end);

	const struct
	{
		bool x, y;
	} in = {
		sorted.begin.x <= x && x <= sorted.end.x,
		sorted.begin.y <= y && y <= sorted.end.y
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

			if(y == sorted.begin.y)
				return x >= sorted.begin.x;
			if(y == sorted.end.y)
				return x <= sorted.end.x;

			return true;
	}

	return false;
}
