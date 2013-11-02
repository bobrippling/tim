#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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

bool isallspace(const char *s)
{
	for(; *s && isspace(*s); s++);
	return !*s;
}

static const struct paren
{
	char opp;
	bool lead;
} parens[] = {
	['['] = { ']', true },
	['{'] = { '}', true },
	['('] = { ')', true },
	['<'] = { '>', true },
	[']'] = { '[', false },
	['}'] = { '{', false },
	[')'] = { '(', false },
	['>'] = { '<', false },
};

static const struct paren *paren_lookup(char c)
{
	unsigned char u = c;

	if(u >= sizeof parens / sizeof *parens)
		return &parens[0];
	return &parens[u];
}

bool paren_match(char c, char *other)
{
	return (*other = paren_lookup(c)->opp);
}

bool paren_left(char c)
{
	return paren_lookup(c)->lead;
}

char paren_opposite(char c)
{
	return paren_lookup(c)->opp;
}

void str_ltrim(char *s, size_t *pl)
{
	size_t i;
	for(i = 0; i < *pl && isspace(s[i]); i++);

	if(i)
		memmove(s, s + i, *pl -= i);
}

void str_rtrim(char *s, size_t *pl)
{
	if(!*pl)
		return;

	size_t i = *pl - 1;
	while(isspace(s[i]))
		if(--i == 0)
			break;

	*pl = i + 1;

	if(isspace(s[i + 1]))
		s[i + 1] = '\0';
}
