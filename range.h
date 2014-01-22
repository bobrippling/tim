#ifndef CMD_RANGE_H
#define CMD_RANGE_H

#include <stdbool.h>

struct range
{
	int start, end;
};

bool parse_range(
		char *cmd, char **end,
		struct range *r);

#endif
