#ifndef CMD_RANGE_H
#define CMD_RANGE_H

#include <stdbool.h>

struct range
{
	int start, end;
};
enum range_parse
{
	RANGE_PARSE_NONE,
	RANGE_PARSE_FAIL,
	RANGE_PARSE_PASS,
};

bool parse_range_1(
		char *range, char **end,
		int *out);

enum range_parse parse_range(
		char *cmd, char **end,
		struct range *r);

void range_sort(struct range *);

#endif
