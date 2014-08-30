#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "mem.h"

void *umalloc(size_t l)
{
	void *p = malloc(l);
	if(!p)
		abort();
	memset(p, 0, l);
	return p;
}

void *urealloc(void *p, size_t l)
{
	void *r = realloc(p, l);
	if(!r)
		abort();
	return r;
}

char *ustrdup(const char *s)
{
	char *r = umalloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

char *ustrdup_len(const char *s, size_t len)
{
	char *r = umalloc(len + 1);
	memcpy(r, s, len);
	r[len] = '\0';
	return r;
}

char *join(const char *sep, char **vec, int n)
{
	const int len_sep = strlen(sep);
	int len = 1;

	for(int i = 0; i < n; i++)
		len += len_sep + strlen(vec[i]);

	char *p, *r = p = umalloc(len);

	for(int i = 0; i < n; i++)
		p += sprintf(p, "%s%s", vec[i], sep);
	if(n > 0)
		p[-len_sep] = '\0';

	return r;
}

char *ustrvprintf(const char *fmt, va_list l)
{
	char *buf = NULL;
	int len = 8, ret;

	do{
		va_list lcp;

		len *= 2;
		buf = urealloc(buf, len);

		va_copy(lcp, l);
		ret = vsnprintf(buf, len, fmt, lcp);
		va_end(lcp);

	}while(ret >= len);

	return buf;

}
