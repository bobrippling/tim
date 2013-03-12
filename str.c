#include <stdlib.h>

#include "str.h"

char *strchr_rev(const char *s, int ch, const char *start)
{
	while(*s != ch){
		if(s == start)
			return NULL;
		s--;
	}
	return (char *)s; /* *s == ch */
}
