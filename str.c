#include <stdlib.h>
#include <ctype.h>

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

int isallspace(const char *s)
{
	for(; *s && isspace(*s); s++);
	return !*s;
}
