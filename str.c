#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "str.h"
#include "mem.h"

char *strchr_rev(const char *s, int ch, const char *start)
{
	while(*s != ch){
		if(s == start)
			return NULL;
		s--;
	}
	return (char *)s; /* *s == ch */
}

char *tim_strrevstr(char *restrict haystack, unsigned off, const char *restrict needle)
{
	const size_t nlen = strlen(needle);

	for(char *p = haystack + off;
			p >= haystack;
			p--)
	{
		if(!strncmp(p, needle, nlen))
			return p;
	}

	return NULL;
}

static char *tim_strstr_mask(
		char *restrict haystack, size_t len, const char *restrict needle,
		int mask(int))
{
	if(!*needle)
		return NULL;

	/* memstr */
	for(size_t h = 0; h < len; h++)
		if(mask(haystack[h]) == mask(*needle))
			/* can assume needle is 0-terminated */
			for(size_t n = 0; n < len - h;){
				if(mask(haystack[h + n]) != mask(needle[n]))
					break;
				n++;
				if(!needle[n])
					return haystack + h;
			}

	return NULL;
}

static int identity(int x)
{
	return x;
}

char *tim_strstr(char *restrict haystack, size_t len, const char *restrict needle)
{
	return tim_strstr_mask(haystack, len, needle, identity);
}

char *tim_strcasestr(
		char *restrict haystack,
		size_t len,
		const char *restrict needle)
{
	return tim_strstr_mask(haystack, len, needle, tolower);
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

int iswordchar(char c)
{
	return isalnum(c) || c == '_';
}

char *word_before(char *line, int x)
{
	int start;
	for(start = x - 1; start > 0;)
		if(iswordchar(line[start - 1]))
			start--;
		else
			break;

	if(start < 0)
		start = 0;

	return ustrdup_len(line + start, x - start);
}
