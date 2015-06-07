#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

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

char *tim_strstr(char *restrict haystack, size_t len, const char *restrict needle)
{
	if(!*needle)
		return NULL;

	/* memstr */
	for(size_t h = 0; h < len; h++)
		if(haystack[h] == *needle)
			/* can assume needle is 0-terminated */
			for(size_t n = 0; n < len - h;){
				if(haystack[h + n] != needle[n])
					break;
				n++;
				if(!needle[n])
					return haystack + h;
			}

	return NULL;
}

bool isallspace(const char *s, size_t len)
{
	for(size_t i = 0; i < len; i++)
		if(!isspace(s[i]))
			return false;
	return true;
}

bool isdigit_or_minus(char c)
{
	return isdigit(c) || c == '-';
}

char *skipspace(const char *in)
{
	for(; isspace(*in); in++);
	return (char *)in;
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
	size_t i = *pl;

	while(i > 0 && isspace(s[i - 1])){
		s[i - 1] = '\0';
		i--;
	}

	*pl = i;
}

char *expand_tilde(const char *in)
{
	if(*in != '~')
		return NULL;

	const char *user = in + 1;

	if(isalpha(*user)){
		const char *user_end;

		for(user_end = user; isalnum(*user_end); user_end++);
		/* TODO */

		return NULL;
	}else{
		/* plain ~ */
		const char *home = getenv("HOME");
		if(!home)
			return NULL;

		size_t homelen = strlen(home);
		size_t userlen = strlen(user);
		size_t len = homelen + userlen + 1;
		char *ret = umalloc(len);

		memcpy(ret, home, homelen);
		memcpy(ret + homelen, user, userlen);
		ret[homelen + userlen] = '\0';
		return ret;
	}
}
