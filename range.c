#include <stdio.h>
#include <stdlib.h>

#include "pos.h"
#include "region.h"
#include "list.h"
#include "buffer.h"
#include "buffers.h"

#include "range.h"

static bool parse_range_1(
		char *range, char **end,
		int *out)
{
	bool handled = true;

	switch(*range){
		case '0' ... '9':
			*out = strtol(range, &range, 10);
			break;

		default:
			handled = false;
			/* fall */

		case '.':
			*out = buffers_cur()->ui_pos->y;
			range++;
			break;
		case '$':
			*out = buffer_nlines(buffers_cur());
			range++;
			break;
		/*case '\'': */
	}

	*end = range;
	for(;;){
		switch(**end){
			case '+':
			case '-':
				range++;
				*out += strtol(range, end, 10);
				handled = true;
				/* read more +/- */
				break;
			default:
				return handled;
		}
	}
}

bool parse_range(
		char *cmd, char **end,
		struct range *r)
{
	if(*cmd == '%'){
		r->start = 0;
		r->end = buffer_nlines(buffers_cur());
		*end = cmd + 1;
		return true;
	}

	if(!parse_range_1(cmd, end, &r->start))
		return false;

	if(*cmd == ','){
		++*cmd;
		if(!parse_range_1(cmd, end, &r->end))
			return false;
	}else{
		r->end = r->start;
	}

	return true;
}

