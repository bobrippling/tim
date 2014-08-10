#ifndef MARK_H
#define MARK_H

#include "pos.h"

typedef struct markgroup markgroup;

struct markgroup
{
	struct mark
	{
		char ch;
		point_t pt;
	};

	struct mark lastmark; /* '' */
};

void mark_add(markgroup *, char, const point_t *);

#endif
