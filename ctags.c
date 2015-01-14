#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mem.h"

#include "ctags.h"

bool ctag_search(const char *ident, struct ctag_result *tag)
{
	FILE *f = fopen("tags", "r");
	if(!f)
		return false;

	bool found = false;

	char buf[256];
	while(fgets(buf, sizeof buf, f)){
		char *end = strchr(buf, '\t');
		if(!end || !end[1])
			continue;
		*end = '\0';
		if(!strcmp(buf, ident)){
			char *fname = end + 1;
			end = strchr(end + 1, '\t');
			if(!end || !end[1])
				continue;
			*end = '\0';

			char *nl = strchr(end + 1, '\n');
			if(nl)
				*nl = '\0';

			tag->fname = ustrdup(fname);
			tag->line = ustrdup(end + 1);
			found = true;
			break;
		}
	}

	fclose(f);
	return found;
}

void ctag_free(struct ctag_result *r)
{
	free(r->fname);
	free(r->line);
	memset(r, 0, sizeof *r);
}
