#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
	const char *sep_actual = "";
	char *r = NULL, *p;
	int i, len = 1;

	for(i = 0; i < n; i++)
		len += len_sep + strlen(vec[i]);

	p = r = umalloc(len);

	for(i = 0; i < n; i++){
		p += sprintf(p, "%s%s", vec[i], sep_actual);
		sep_actual = sep;
	}

	return r;
}
