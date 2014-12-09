#ifndef REGION_H
#define REGION_H

#include <stdbool.h>

typedef struct region
{
	point_t begin, end;
	enum region_type
	{
		REGION_CHAR,
		REGION_COL,
		REGION_LINE
	} type;
} region_t;


bool region_contains(const region_t *, point_t const *);

#endif
