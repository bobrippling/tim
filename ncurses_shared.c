#include <ctype.h>

#include "ncurses.h"

int nc_charlen(int ch)
{
	int l = 1;
	switch(ch){
		case '\r':
			l++; /* ^M */
			break;
		case '\t':
			break;
		default:
			if(!isprint(ch))
				l++; /* ^X */
	}
	return l;
}
