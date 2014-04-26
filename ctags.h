#ifndef CTAGS_H
#define CTAGS_H

#include <stdbool.h>

struct ctag_result
{
	char *fname;
	char *line;
};

bool ctag_search(const char *ident, struct ctag_result *);

void ctag_free(struct ctag_result *);

#endif
