#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"
#include "window.h"
#include "windows.h"

#include "range.h"

bool parse_range_1(
		char *range, char **end,
		int *out)
{
	bool handled = true;

	switch(*range){
		case '0' ... '9':
			/* go into 0-based */
			*out = strtol(range, &range, 10) - 1;
			break;

		case '.':
			*out = windows_cur()->ui_pos->y;
			range++;
			break;
		case '$':
			*out = buffer_nlines(buffers_cur());
			range++;
			break;
		/*case '\'': */

		default:
			/* don't touch range */
			handled = false;
			*out = windows_cur()->ui_pos->y;
			break;
	}

	for(;;){
		switch(*range){
			case '+':
			case '-':
			{
				const int dir = (*range == '+' ? 1 : -1);

				range++;
				int amt = isdigit(*range)
					? strtol(range, &range, 10)
					: 1;

				*out += dir * amt;

				handled = true;
				/* read more +/- */
				break;
			}
			default:
				*end = range;
				return handled;
		}
	}
}

enum range_parse parse_range(
		char *cmd, char **end,
		struct range *r)
{
	if(*cmd == '%'){
		r->start = 0;
		r->end = buffer_nlines(buffers_cur());
		*end = cmd + 1;
		return RANGE_PARSE_PASS;
	}

	if(!parse_range_1(cmd, end, &r->start))
		return RANGE_PARSE_NONE; /* no range */

	cmd = *end;
	if(*cmd == ','){
		cmd++;
		if(!parse_range_1(cmd, end, &r->end))
			return RANGE_PARSE_FAIL;
	}else{
		r->end = r->start;
	}

	return RANGE_PARSE_PASS;
}


void range_sort(struct range *r)
{
	if(r->start > r->end){
		int tmp = r->start;
		r->start = r->end;
		r->end = tmp;
	}
}
